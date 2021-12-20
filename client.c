#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

char *programName;

typedef struct Info
{
    char* port;
    char* file;
    char* dir;
    char* url;
}Info;
//x

void usage(char *message)
{
    fprintf(stdout, "Usage in %s", message);
    exit(EXIT_FAILURE);
}

void argumentParsing(Info *inf, int argc, char *argv[]){

    int p_ind = 0;
    int o_ind = 0;
    int d_ind = 0;

    char *port = NULL;
    char *file = NULL;
    char *dir = NULL;

    int c;
    // Synopsis: client [-p PORT] [ -o FILE | -d DIR ] URL
    while ((c = getopt(argc, argv, "p:o:d:")) != -1)
    {
        switch (c)
        {
        case 'p':
            p_ind++;
            port = optarg;
            break;
        case 'o':
            if (d_ind > 0)
            {
                usage(programName);
            }
            o_ind++;
            file = optarg;
            break;
        case 'd':
            if (o_ind > 0)
            {
                usage(programName);
            }
            d_ind++;
            dir = optarg;
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

    inf ->dir = dir;
    inf ->file = file;
    inf ->url = argv[optind];
    if(port == NULL){
        inf->port = "80";
    }
    free(dir);
    free(file);
    free(port);
    

}

int main(int argc, char *argv[])
{
    programName = argv[0];
    Info info;
    // Synopsis: client [-p PORT] [ -o FILE | -d DIR ] URL
    argumentParsing(&info,argc,argv);

    //TODO: disect URL
    //TODO: create Socket
    //TODO: write request to Socket
    //TODO: read from Socket and print [to file/dir]

}