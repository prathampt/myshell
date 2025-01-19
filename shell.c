#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define N 1024 /* Max input buffer size for shell */

void strstrip(char *s)
{
    int size = strlen(s), i = 0, j = 0;
    if (!size)
        return;

    char *end = s + size - 1;

    while (end >= s && *end == ' ')
        end--;
    *(end + 1) = '\0';

    while (s[i] == ' ')
        i++;
    while (s[j++] = s[i++])
        ;

    return;
}

int main()
{
    char input[N], prompt[N];
    int userPrompt = 0;
    char *eof;

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
                printf("Usuage: PS1=\"whatever string you want\" or PS1=\"\\w$\" to restore default");
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
                    printf("Usuage: PS1=\"whatever string you want\" or PS1=\"\\w$\" to restore default");
                }
                else
                {
                    strcpy(prompt, temp);
                    userPrompt = 1;
                    prompt[t] = 0;
                }
            }
        }

        else if (!strncmp(input, "cd ", 3))
        {
            chdir(input + 3);
        }

        else
        {
            printf("%s\n", input); /* Expected to write function to handle inputs here... */
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