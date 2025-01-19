#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define N 1024 /* Max input buffer size for shell */

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

        if (!strcmp(input, "exit"))
        {
            break;
        }

        if (!strncmp(input, "PS1=", 4))
        {
            if (!strncmp(input + 5, "\\w$", 3)) {
                userPrompt = 0;
            }
            else {
                userPrompt = 1;
                strcpy(prompt, input + 5);
                prompt[strcspn(prompt, "\"")] = 0;
            }

        }

        printf("%s\n", input); /* Expected to write function to handle inputs here... */

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