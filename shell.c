#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/wait.h>
#include <fcntl.h>

#define N 1024 /* Max input buffer size for shell */
extern int errno;

void strstrip(char *s)
{
    int size = strlen(s), i = 0, j = 0;
    if (!size)
        return;

    char *end = s + size - 1;

    while (end >= s && (*end == ' ' || *end == '\t'))
        end--;
    *(end + 1) = 0;

    while (s[i] == ' ' || s[i] == '\t')
        i++;

    /* To remove the spaces in between the arguments, and make the input clean... */

    int inSpace = 0;
    while (s[i])
    {
        if (s[i] == ' ' || s[i] == '\t')
        {
            if (!inSpace)
            {
                s[j++] = ' ';
                inSpace = 1;
            }
        }
        else
        {
            s[j++] = s[i];
            inSpace = 0;
        }
        i++;
    }

    s[j] = 0;

    return;
}

char **parse(char *input)
{
    char **args = (char **)malloc((N / 2 + 1) * sizeof(char *)); /* To hold the command and the arguments...*/
    int i = 0;
    char *token = input;

    while (token && *token != '\0')
    {
        while (*token == ' ' || *token == '\t')
        {
            token++;
        }

        if (*token == '\0')
        {
            break;
        }

        if (*token == '"' || *token == '\'')
        {
            char quoteType = *token;
            token++;
            char *start = token;

            while (*token && *token != quoteType)
            {
                token++;
            }

            if (*token == quoteType)
            {
                *token = '\0';
                args[i++] = strdup(start);
                token++;
            }
            else
            {
                fprintf(stderr, "\033[0;31mError: Unmatched quote\033[0m\n");
                return NULL;
            }
        }
        else
        {
            char *start = token;

            while (*token && *token != ' ' && *token != '\t')
            {
                token++;
            }

            if (*token)
            {
                *token = '\0';
                token++;
            }

            if (*start == '~')
            {
                char *newString = (char *)malloc((strlen(start) + strlen(getenv("HOME")) + 1) * sizeof(char));
                if (newString)
                {
                    strcpy(newString, getenv("HOME"));
                    strcat(newString, "/");
                    strcat(newString, start + 1);
                    args[i++] = newString;
                }
            }
            else
            {
                args[i++] = strdup(start);
            }
        }
    }

    args[i] = NULL;

    return args;
}

int setRedirection(char *input, int *redirect0, int *redirect1, int *redirect2, int *outputInAppendMode, char *in, char *out, char *err)
{
    int i = 0;
    *redirect0 = 0, *redirect1 = 0, *redirect2 = 0, *outputInAppendMode = 0;
    in[0] = 0, out[0] = 0, err[0] = 0;

    while (input[i])
    {
        if ((input[i] == '&' && input[i + 1] && input[i + 1] == '>') || (input[i] == '>' && input[i + 1] && input[i + 1] == '&'))
        {
            *redirect2 = 1;
            *redirect1 = 1;
            input[i++] = ' ';
            input[i++] = ' ';
            if (input[i] == ' ')
                i++;
            int j = 0;
            while (input[i] && input[i] != ' ')
            {
                err[j++] = input[i];
                input[i++] = ' ';
            }
            err[j] = 0;
            strcpy(out, err);
        }
        if (input[i] == '<')
        {
            *redirect0 = 1;
            input[i++] = ' ';
            if (input[i] == ' ')
                i++;
            int j = 0;
            while (input[i] && input[i] != ' ')
            {
                in[j++] = input[i];
                input[i++] = ' ';
            }
            in[j] = 0;
        }
        if (input[i] == '>')
        {
            *redirect1 = 1;
            input[i++] = ' ';
            if (input[i] == '>')
            {
                *outputInAppendMode = 1;
                input[i++] = ' ';
            }
            if (input[i] == ' ')
                i++;
            int j = 0;
            while (input[i] && input[i] != ' ')
            {
                out[j++] = input[i];
                input[i++] = ' ';
            }
            out[j] = 0;
        }
        if (input[i] == '2' && input[i + 1] && input[i + 1] == '>')
        {
            *redirect2 = 1;
            input[i++] = ' ';
            input[i++] = ' ';
            if (input[i] == ' ')
                i++;
            int j = 0;
            while (input[i] && input[i] != ' ')
            {
                err[j++] = input[i];
                input[i++] = ' ';
            }
            err[j] = 0;
        }
        i++;
    }

    return 0;
}

int execWrapper(char *command, char *args[], char *path)
{
    if (!path)
        return -1;

    if (strchr(command, '/') != NULL)
    {
        /* If the command contains a '/' it is assumed that the absolute path is provided... */
        return execv(command, args);
    }
    char fullPath[N];
    char *pathCopy = strdup(path); /* So that the original path is not changed (underlying malloc is called) */
    char *dir = strtok(pathCopy, ":");

    while (dir != NULL)
    {
        strcpy(fullPath, dir);
        strcat(fullPath, "/");
        strcat(fullPath, command);

        if (execv(fullPath, args) == 0)
        {
            free(pathCopy);
            return 0;
        }
        dir = strtok(NULL, ":");
    }

    free(pathCopy);
    errno = ENOENT;
    return -1;
}

int execute(char *input, char *path, int redirect0, int redirect1, int redirect2, int outputInAppendMode, char *in, char *out, char *err)
{
    char **args = parse(input);

    pid_t pid = fork();

    if (pid == 0)
    {
        if (redirect0)
        {
            close(0);
            if (open(in, O_RDONLY) == -1)
            {
                perror("");
                return -1;
            }
        }

        if (outputInAppendMode && redirect1)
        {
            close(1);
            if (open(out, O_WRONLY | O_APPEND | O_CREAT, 0644) == -1)
            {
                perror("");
                return -1;
            }
        }
        else if (redirect1)
        {
            close(1);
            if (open(out, O_WRONLY | O_CREAT, 0644) == -1)
            {
                perror("");
                return -1;
            }
        }

        if (redirect2)
        {
            close(2);
            if (open(err, O_WRONLY | O_CREAT, 0644) == -1)
            {
                perror("");
                return -1;
            }
        }

        if (execWrapper(args[0], args, path) == -1)
        {
            perror("execute");
            exit(errno);
        }
    }
    else if (pid > 0)
    {
        wait(NULL);
    }
    else
    {
        perror("fork failed");
        exit(errno);
    }

    return 0;
}

int main()
{
    char input[N], prompt[N];
    int userPrompt = 0; /* 1 - Use prompt given by user; 0 - Use default prompt (cwd) */
    char *eof;
    char *path = getenv("PATH");
    int redirect0 = 0, redirect1 = 0, redirect2 = 0, outputInAppendMode = 0;
    char *in, *out, *err;
    in = (char *)malloc(N * sizeof(char));
    out = (char *)malloc(N * sizeof(char));
    err = (char *)malloc(N * sizeof(char));

    getcwd(prompt, N);
    printf("\033[1;36m%s\033[1;37m$\033[0m ", prompt);

    while (eof = fgets(input, N, stdin)) /* Returns NULL when gets EOF */
    {
        input[strcspn(input, "\n")] = 0;

        strstrip(input);

        if (!strcmp(input, "exit"))
            break;

        else if (!strncmp(input, "PS1=", 4))
        {

            if (strncmp(input, "PS1=\"", 5))
            {
                printf("\033[0;31mUsuage:\033[0m PS1=\"whatever string you want\" or PS1=\"\\w$\" to restore default\n");
            }

            else if (!strncmp(input + 5, "\\w$", 3))
                userPrompt = 0;

            else
            {
                char temp[N];
                strcpy(temp, input + 5);
                int t = strcspn(temp, "\"");
                if (t >= strlen(temp))
                {
                    printf("\033[0;31mUsuage:\033[0m PS1=\"whatever string you want\" or PS1=\"\\w$\" to restore default\n");
                }
                else
                {
                    strcpy(prompt, temp);
                    userPrompt = 1;
                    prompt[t] = 0;
                }
            }
        }

        else if (!strncmp(input, "PATH=", 5))
        {

            if (strncmp(input, "PATH=\"", 6) != 0)
            {
                printf("\033[0;31mUsuage:\033[0m PATH=\"paths/to/executables\"\n");
            }

            else
            {
                char temp[N];
                strcpy(temp, input + 6);
                int t = strcspn(temp, "\"");
                if (t >= strlen(temp))
                {
                    printf("\033[0;31mUsuage:\033[0m PATH=\"paths/to/executables\"\n");
                }
                else
                {
                    strcpy(path, temp);
                    path[t] = 0;
                }
            }
        }

        else if (!strncmp(input, "cd", 2))
        {
            if (*(input + 2) == 0)
            {
                if (chdir(getenv("HOME")) == -1)
                    perror("cd");
            }
            else if (*(input + 2) == ' ' && *(input + 3) == '~')
            {
                if (chdir(getenv("HOME")) == -1)
                    perror("cd");
                if (*(input + 4) && *(input + 4) == '/')
                {
                    if (*(input + 5) && chdir(input + 5) == -1)
                        perror("cd");
                }
            }
            else if (*(input + 2) == ' ')
            {
                if (chdir(input + 3) == -1)
                    perror("cd");
            }
        }

        else
        {
            setRedirection(input, &redirect0, &redirect1, &redirect2, &outputInAppendMode, in, out, err); /* This function is going to modify the input and set redirection related flags */
            strstrip(input);
            if (execute(input, path, redirect0, redirect1, redirect2, outputInAppendMode, in, out, err) == -1)
            {
                perror("myshell");
            }
        }

        if (userPrompt)
        {
            printf("\033[1;32m%s\033[0m ", prompt);
        }
        else
        {
            getcwd(prompt, N);
            printf("\033[1;36m%s\033[1;37m$\033[0m ", prompt);
        }
    }

    if (!eof)
    {
        printf("\n"); /* Just to mimic the behaviour of an actual shell */
        /* return the prompt in both the cases on a new line... */
    }

    return 0;
}