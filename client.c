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
    char* dir;
    char* url;
}Info;

typedef struct DisURL
{
    char *host;
    char *filepath;
    int requestPath;

}DisURL;

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
    
}

void dissectURL(DisURL *dissectedUrl, char* url){
    int s;
    if((s = strncmp(url,"http://",7)) != 0){
        usage(programName);
    }
    char* nptr = NULL;
    
    dissectedUrl->requestPath = 0;
    if(url[strlen(url) -1] == '/'){
        dissectedUrl->requestPath = 1;
    }

    strtok(url,"://");
    dissectedUrl->host = strtok(nptr,";/:@=&?");
    dissectedUrl->filepath = strtok(nptr,"\0");
}

int setupSocket(char* host, char* port){

    struct addrinfo hints, *ai;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    getaddrinfo(host ,port , &hints, &ai);

    int sockfd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
    connect(sockfd, ai->ai_addr, ai->ai_addrlen);
    freeaddrinfo(ai);
    return sockfd;
}

void readFromSocket(FILE *sockfile, DisURL *disURL, char* dest){
    char *host = disURL->host;
    char *file = disURL->filepath;

    if(file == NULL){
        file = "";
    }
    fprintf(sockfile, "GET /%s HTTP/1.1\r\nHost: %s\r\nConnection: close \r\n\r\n",file ,host);
    fflush(sockfile);
    
    char* header = NULL;
    size_t len = 0;
    getline(&header, &len, sockfile);

    char* nptr = NULL;
    char* tmp = NULL;

    tmp = strtok(header," ");
    if(strcmp(tmp,"HTTP/1.1") != 0){
        fprintf(stderr,"Protocol error.");
        exit(2);
    }

    tmp = strtok(nptr," ");
    int c;
    char* endptr;
    if((c = strtol(tmp,&endptr,10)) == 0){
        fprintf(stderr,"Protocol error");
        exit(2);
    }

    if(strcmp(tmp,"200") != 0){
        fprintf(stderr,"%s",tmp);
        exit(3);
    }

    FILE *destfile;

    if(dest != NULL){
        if((destfile = fopen(dest,"w")) == NULL){
            fprintf(stderr,"file open Error %s",dest);
            exit(EXIT_FAILURE);
        }
    }

    else{
       destfile = stdout;
    }

    char buf[1024];
    while ((fgets(buf, sizeof(buf), sockfile)) != NULL)
        fputs(buf, destfile);
    
    fflush(destfile);
    fclose(destfile);
    free(header);
}

int main(int argc, char *argv[])
{   
    programName = argv[0];
    Info info;
    // Synopsis: client [-p PORT] [ -o FILE | -d DIR ] URL
    argumentParsing(&info,argc,argv);
    DisURL dissectedURL;
    dissectURL(&dissectedURL,info.url);

    int sockfd = setupSocket(dissectedURL.host, info.port);
    FILE *sockfile = fdopen(sockfd, "r+");
    
    char* file = NULL;

    if(dissectedURL.requestPath == 1 && info.dir == NULL && info.file == NULL){
        file = "index.html";
    }

    if(info.file != NULL){
        file = info.file;
    }

    if(info.dir != NULL){
        file = (char*) malloc(sizeof(info.dir) * 2 + 1);

        strcpy(file,info.dir);
        strcat(file,"/");
        strcat(file,info.dir);
    }

    readFromSocket(sockfile,&dissectedURL,file);

    fclose(sockfile);
    if(info.dir != NULL) free(file);
    //TODO: read from Socket and print [to file/dir]/
    

}