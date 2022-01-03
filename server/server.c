#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <errno.h>
#include <time.h>

char *programName;

typedef struct Info
{
    char *port;
    char *file;
    char *doc_root;
} Info;

typedef struct Header
{
    char* req_method; 
    char* path; 
    char* version; 
    char* empty; 
} header;

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


char* getDate(){
    char* date = malloc(100);
    time_t t = time(NULL); 
    struct tm *ti = localtime(&t);
    
    strftime(date, 100, "%a, %d %b %y %T %Z", ti);
    return date;
}

int getFileSize(char* file){
    FILE *f;
    if((f = fopen(file, "r")) == NULL){
        return -1;
    }
    if((fseek(f,-1L,SEEK_END)) == -1){
        fclose(f);
        return -1;
    }
    int size = ftell(f);
    fclose(f);
    return size;
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

static struct Header extractHeader(FILE *sockfile){

    char* request_header;
    size_t len = 0;
    if(getline(&request_header,&len,sockfile) == -1){
        fprintf(stderr, "failed at reading request Header");
        fclose(sockfile);
        exit(EXIT_FAILURE);
    }
    struct Header head;
    head.req_method = strtok(request_header, " ");
    head.path = strtok(NULL, " ");
    head.version = strtok(NULL, " ");
    head.empty = strtok(NULL, "\r\n");
    return head;
}

int parseHeader(header *head){

    int msgCode = 200;
    //check request
    if(head->req_method == NULL || head->path == NULL || head->version == NULL || head->empty != NULL){
        msgCode = 400;
    }
    if(strcmp(head->version, "HTTP/1.1\r\n") != 0){
        msgCode = 400;
    }
    if(strcmp(head->req_method, "GET") != 0){
        msgCode = 501;
    }
    return msgCode;
}

void skipBody(FILE *sockfile){

    char* npt = NULL;
    size_t len_npt = 0;

    do{
        getline(&npt,&len_npt, sockfile);
    }
    while((strcmp(npt,"\r\n")) != 0);

}

void writeContent(FILE *sockfile,FILE *content ){

}


void chat(int sockfd, char* doc_root){

    fprintf(stderr, "Before Accept %s",programName); //TODO remove line
    //accept connection
    if ((sockfd = accept(sockfd, NULL, NULL)) < 0)
    {
        fprintf(stderr, "failed at accept");
        exit(EXIT_FAILURE);
    }
    fprintf(stderr, "Connection accepted %s",programName); //TODO remove line


    //open socketfile
    FILE *sockfile;
    if ((sockfile = fdopen(sockfd, "w+")) == NULL)
    {
        fprintf(stderr, "failed at opening File");
        exit(EXIT_FAILURE);
    }

    //read request header

    struct Header request_header;
    request_header = extractHeader(sockfile);

    //parse header, correct if 200
    int msgCode = 0;
    msgCode = parseHeader(&request_header);

    //skip rest of file
    skipBody(sockfile);


    char f_path[strlen(doc_root) + strlen(request_header.path)];
    strcpy(f_path,doc_root);
    strcat(f_path, request_header.path);

    fprintf(stderr, "\nFILEPATH %s \n",f_path); 
    FILE *content = fopen(f_path, "r");
    if(content == NULL && msgCode == 200){
        msgCode = 404;
    }
    if(msgCode != 200){
        fprintf(sockfile,"HTTP/1.1 %d \r\nConnection: close\r\n\r\n", msgCode);
        fflush(sockfile);
    }
    else{
        int length = getFileSize(f_path);
        char* date = getDate();
        char* status = "OK";

        if(fprintf(sockfile, "HTTP/1.1 %d %s\r\nDate: %s\r\nContent-Length: %d\r\nConnection: close\r\n\r\n", msgCode, status, date, length) < 0){
            fprintf(stderr, "failed at print in %s", programName);
        }
    }

    char* line = NULL;
    size_t line_length = 0;
    fflush(sockfile); 
    while((getline(&line, &line_length, content)) != -1){
        fprintf(sockfile,"%s",line);
    }    

    fclose(sockfile);
    fclose(content);

}




int main(int argc, char *argv[])
{
    programName = argv[0];
    Info info;
    argumentParsing(&info, argc, argv);
    fprintf(stderr, "PORT: %s",info.port);
    int sockfd = setupSocket(info.port);
    printf("after Setup");
    getDate();
    if(sockfd < 0){
        fprintf(stderr, "failed at socket setup, error: %s in %s", strerror(errno), programName);
        exit(EXIT_FAILURE);
    }

    chat(sockfd, info.doc_root);
    close(sockfd);
}
//TODO: add index.html
//TODO: signal handling
//TODO: accept more connections