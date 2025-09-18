#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))


int main(int argc, char **argv)
{
    /*
        fread/fwrite = reading/writing bytes
        scanf/fscanf = reading and copying the format to a given var
    */
    return EXIT_SUCCESS;
}