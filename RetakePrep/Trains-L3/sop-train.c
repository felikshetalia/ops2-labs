#include <stdio.h>
#include <stdlib.h>

void usage(const char* exec_name)
{
    printf("%s name from to speed - simulate train running on railway network\n", exec_name);
}

int main(int argc, char* argv[])
{
    // STAGE3 and STAGE4
    if (argc != 5)
    {
        usage(argv[0]);
        exit(EXIT_FAILURE);
    }

    printf("Hello from train! Implement me!\n");

    return EXIT_SUCCESS;
}