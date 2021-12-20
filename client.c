#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

char *programName;

void usage(char *message)
{
    fprintf(stdout, "Usage in %s", message);
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[])
{
    programName = argv[0];

    int p_ind = 0;
    int o_ind = 0;
    int d_ind = 0;

    char *p_arg = NULL;
    char *o_arg = NULL;
    char *d_arg = NULL;

    int c;
    // Synopsis: client [-p PORT] [ -o FILE | -d DIR ] URL
    while ((c = getopt(argc, argv, "p:o:d:")) != -1)
    {
        switch (c)
        {
        case 'p':
            p_ind++;
            p_arg = optarg;
            break;
        case 'o':
            if (d_ind > 0)
            {
                usage(programName);
            }
            o_ind++;
            o_arg = optarg;
            break;
        case 'd':
            if (o_ind > 0)
            {
                usage(programName);
            }
            d_ind++;
            d_arg = optarg;
            break;
        default:
            usage(programName);
        }
    }

    if (p_ind > 1 || o_ind > 1 || d_ind > 1 || (o_ind > 0 && d_ind > 0))
    {
        usage(programName);
    }
    
    if (((p_ind * 2) + (o_ind * 2) + (d_ind * 2)) != argc - 2)
    {
        usage(programName);
    }
}