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
    if(file == NULL){
        file = "index.html";
    }
    inf->file = file;
    inf->doc_root = argv[optind];
    if (port == NULL)
    {
        port = "8080";
    }
    inf->port = port;
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
    if ((res = bind(sockfd, ai->ai_addr, ai->ai_addrlen)) < 0) //TODO: fix "Adress already in use Error, cause (?)"
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

    fprintf(stderr, "Before Accept"); //TODO remove line
    if ((sockfd = accept(sockfd, NULL, NULL)) < 0)
    {
        fprintf(stderr, "failed at accept");
        exit(EXIT_FAILURE);
    }
    fprintf(stderr, "Connection accepted"); //TODO remove line



    FILE *sockfile;
    if ((sockfile = fdopen(sockfd, "w+")) == NULL)
    {
        fprintf(stderr, "failed at opening File");
        exit(EXIT_FAILURE);
    }

    //read request header
    char* request_header;
    size_t len = 0;
    if(getline(request_header,&len,sockfile) == -1){
        fprintf(stderr, "failed at reading request Header");
        fclose(sockfile);
        exit(EXIT_FAILURE);
    }
    char* req_method = strtok(request_header, " ");
    char* path = strtok(NULL, " ");
    char* version = strtok(NULL, " ");
    char* empty = strtok(NULL, "\r\n");

    //TODO Continue here

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
    printf("after Setup");
    if(sockfd < 0){
        fprintf(stderr, "failed at socket setup, error: %s in %s", strerror(errno), programName);
        exit(EXIT_FAILURE);
    }

    chat(sockfd);
    close(sockfd);
}