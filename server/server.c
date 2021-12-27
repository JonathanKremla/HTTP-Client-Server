#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>

char *programName;

typedef struct Info
{
    char* port;
    char* file;
    char* doc_root;
}Info;

void usage(char *message)
{
    fprintf(stdout, "Usage in %s", message);
    exit(EXIT_FAILURE);
}


void argumentParsing(Info *inf, int argc, char *argv[]){

    int p_ind = 0;
    int i_ind = 0;

    char *port = NULL;
    char *file = NULL;
    int c;
    // Synopsis: server [-p PORT] [-i INDEX] DOC_ROOT 
    while ((c = getopt(argc, argv, "p:i:"))!= -1)
    {
        switch (c)
        {
        case 'p':
            p_ind++;
            port = optarg;
            break;
        case 'i':
            i_ind++;
            file = optarg;
            break;
        default:
            usage(programName);
        }
    }

    if(p_ind > 0 || i_ind > 0) 
    {
        usage(programName);
    }
    
    if ((i_ind * 2) + (p_ind * 2) != argc - 2)
    {
        usage(programName);
    }

    inf ->file = file;
    inf ->doc_root = argv[optind];
    if(port == NULL){
        inf->port = "8080";
    }
    
}

int main (int argc, char *argv[]){
    programName = argv[0];
    Info info;
    argumentParsing(&info, argc, argv);

}