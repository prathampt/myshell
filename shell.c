#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define N 1024 /* Max input buffer size for shell */

int main()
{
    system("clear");

    char *input;

    while (fgets(input, N, stdin)) /* Gracefully terminate shell with EOF */
    {
        input[strcspn(input, "\n")] = 0;

        if (!strcmp(input, "exit")) /* Gracefully terminate shell with "exit" */
        {
            break;
        }

        printf("%s\n", input);
    }

    return 0;
}