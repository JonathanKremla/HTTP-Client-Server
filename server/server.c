#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <errno.h>

char *programName;

typedef struct Info
{
    char *port;
    char *file;
    char *doc_root;
} Info;

void usage(char *message)
{
    fprintf(stdout, "Usage in %s", message);
    exit(EXIT_FAILURE);
}

void argumentParsing(Info *inf, int argc, char *argv[])
{

    int p_ind = 0;
    int i_ind = 0;

    char *port = NULL;
    char *file = NULL;
    int c;
    // Synopsis: server [-p PORT] [-i INDEX] DOC_ROOT
    while ((c = getopt(argc, argv, "p:i:")) != -1)
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

    if (p_ind > 1 || i_ind > 1)
    {
        usage(programName);
    }

    if ((i_ind * 2) + (p_ind * 2) != argc - 2)
    {
        usage(programName);
    }

    inf->file = file;
    inf->doc_root = argv[optind];
    if (port == NULL)
    {
        inf->port = "8080";
    }
    else{
        inf->port = port;
    }
}

static int setupSocket(char *port){

    struct addrinfo hints, *ai;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    int res = getaddrinfo(NULL, port, &hints, &ai);
    if (res != 0)
    {
        fprintf(stderr ,"at getaddrinfo %s ",port);
        return -1;
    }
    int sockfd = socket(ai->ai_family, ai->ai_socktype,
                        ai->ai_protocol);


    if (sockfd < 0)
    {
        fprintf(stderr, "at socket");
        return -1;
    }
    int optval = 1;
    if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval) != 0){
        fprintf(stderr, "at setsockopt ");
        return -1;
        
    }

    if ((res = bind(sockfd, ai->ai_addr, ai->ai_addrlen)) < 0)
    {
        fprintf(stderr, "at bind");
        return -1;
    }
    int temp;
    if ((temp = listen(sockfd, 128)) == -1)
    {
        fprintf(stderr, "at listen");
        return -1;
    }
    freeaddrinfo(ai);
    return sockfd;
}

void chat(int sockfd){


    if ((sockfd = accept(sockfd, NULL, NULL)) < 0)
    {
        fprintf(stderr, "failed at accept");
        exit(EXIT_FAILURE);
    }
    FILE *sockfile;
    if ((sockfile = fdopen(sockfd, "w+")) == NULL)
    {
        fprintf(stderr, "failed at opening File");
        exit(EXIT_FAILURE);
    }
    char buf[1024];
    while ((fgets(buf, sizeof(buf), sockfile)) != NULL)
        fputs(buf, stdout);
}

int main(int argc, char *argv[])
{
    programName = argv[0];
    Info info;
    argumentParsing(&info, argc, argv);
    fprintf(stderr, "PORT: %s",info.port);
    int sockfd = setupSocket(info.port);
    if(sockfd < 0){
        fprintf(stderr, "failed at socket setup, error:%s in %s", strerror(errno), programName);
        exit(EXIT_FAILURE);
    }

    for (;;)
    {
        chat(sockfd);
    }
}