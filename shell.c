#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/wait.h>

#define N 1024 /* Max input buffer size for shell */
extern int errno;

void strstrip(char *s)
{
    int size = strlen(s), i = 0, j = 0;
    if (!size)
        return;

    char *end = s + size - 1;

    while (end >= s && *end == ' ')
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
    return -1;
}

int execute(char *input, char *path)
{
    char *args[N / 2 + 1]; /* To hold the command and the arguments...*/

    int i = 0;

    char *token = strtok(input, " \t");

    while (token != NULL)
    {
        args[i++] = token;
        token = strtok(NULL, " \t");
    }

    args[i] = NULL;

    pid_t pid = fork();

    if (pid == 0)
    {
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

    getcwd(prompt, N);
    printf("%s$ ", prompt);

    while (eof = fgets(input, N, stdin)) /* Returns NULL when gets EOF */
    {
        input[strcspn(input, "\n")] = 0;

        strstrip(input);

        if (!strcmp(input, "exit"))
            break;

        if (!strncmp(input, "PS1=", 4))
        {

            if (strncmp(input, "PS1=\"", 5))
            {
                printf("Usuage: PS1=\"whatever string you want\" or PS1=\"\\w$\" to restore default\n");
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
                    printf("Usuage: PS1=\"whatever string you want\" or PS1=\"\\w$\" to restore default\n");
                }
                else
                {
                    strcpy(prompt, temp);
                    userPrompt = 1;
                    prompt[t] = 0;
                }
            }
        }

        if (!strncmp(input, "PATH=", 5))
        {

            if (strncmp(input, "PATH=\"", 6) != 0)
            {
                printf("Usuage: PATH=\"paths/to/executables\"\n");
            }

            else
            {
                char temp[N];
                strcpy(temp, input + 6);
                int t = strcspn(temp, "\"");
                if (t >= strlen(temp))
                {
                    printf("Usuage: PATH=\"paths/to/executables\"\n");
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
            if (*(input + 2) == 0 && chdir(getenv("HOME")) == -1)
            {
                perror("cd");
            }
            else if (*(input + 2) == ' ' && chdir(input + 3) == -1)
            {
                perror("cd");
            }
        }

        else
        {
            if (execute(input, path) == -1)
            {
                perror("myshell");
            }
        }

        if (userPrompt)
        {
            printf("%s ", prompt);
        }
        else
        {
            getcwd(prompt, N);
            printf("%s$ ", prompt);
        }
    }

    if (!eof)
    {
        printf("\n"); /* Just to mimic the behaviour of an actual shell */
        /* return the prompt in both the cases on a new line... */
    }

    return 0;
}