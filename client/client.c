/**
 * @file client.c
 * @author Jonathan Kremla
 * @brief client recieving data from server through a socket  
 * @date 2021-12-25
 * 
 */
#include <errno.h>
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
    int optDir; //if 1 option -d was given
}Info;

typedef struct DisURL
{
    char *host;
    char *filepath;
    char *saveFile;
    int requestPath; //if 1 Url ends with / thus, requesting a directory

}DisURL;
/**
 * @brief print usage message and exit with exit Code 1 (Failure) 
 * 
 */
void usage()
{
    fprintf(stderr, "Usage in %s: client [-p PORT] [ -o FILE | -d DIR ] URL", programName);
    exit(EXIT_FAILURE);
}
/**
 * @brief function for handeling argument parsing 
 * 
 * @param inf Info struct for saving optional arguments 
 * @param argc argument count 
 * @param argv argument vector 
 */
void argumentParsing(Info *inf, int argc, char *argv[]){

    int p_ind = 0;
    int o_ind = 0;
    int d_ind = 0;
    inf->optDir = 0;

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
                usage();
            }
            o_ind++;
            file = optarg;
            break;
        case 'd':
            if (o_ind > 0)
            {
                usage();
            }
            d_ind++;
            inf->optDir = 1;
            dir = optarg;
            break;
        default:
            usage();
        }
    }

    if (p_ind > 1 || o_ind > 1 || d_ind > 1 || (o_ind > 0 && d_ind > 0))
    {
        usage();
    }
    
    if (((p_ind * 2) + (o_ind * 2) + (d_ind * 2)) != argc - 2)
    {
        usage();
    }

    inf ->dir = dir;
    inf ->file = file;
    inf ->url = argv[optind];
    inf -> port = port;
    if(port == NULL){
        inf->port = "80";
    }
}
/**
 * @brief function for splitting URL into host and filepath
 * 
 * @param dissectedUrl Struct of DisUrl for saving host and filepath 
 * @param url String of given url 
 */
void dissectURL(DisURL *dissectedUrl, char* url, int optDir){
    int s;
    if((s = strncmp(url,"http://",7)) != 0){
        usage();
    }
    char* nptr = NULL;
    
    dissectedUrl->requestPath = 0;
    if(url[strlen(url) -1] == '/'){
        dissectedUrl->requestPath = 1;
    }

    strtok(url,"://");
    dissectedUrl->host = strtok(nptr,";/:@=&?");
    dissectedUrl->filepath = strtok(nptr,"\0");

    if(dissectedUrl->requestPath == 0 && optDir == 1){
        dissectedUrl->saveFile = (strrchr(dissectedUrl->filepath,'/'));
        if(dissectedUrl->saveFile == NULL){
            dissectedUrl->saveFile = dissectedUrl->filepath; //case: http://HOST/example 
        }
    }
    if(dissectedUrl->requestPath == 1 && optDir == 1){
        dissectedUrl->saveFile = "index.html";
    }
}
/**
 * @brief setup new Socket. 
 * 
 * @param host  specifies the host address
 * @param port  specifies port (default = 80)
 * @return int 
 */
int setupSocket(char* host, char* port){

    struct addrinfo hints, *ai;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;


    int res = getaddrinfo(host ,port , &hints, &ai);
    if(res != 0){
        return -1;
    }

    int sockfd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);

    if(sockfd < 0){
        return -1;
    }
    if(connect(sockfd, ai->ai_addr, ai->ai_addrlen) != 0){
        freeaddrinfo(ai);
        return -1;
    }
    freeaddrinfo(ai);
    return sockfd;
}
/**
 * @brief reading from Socket and writing to the specified destination 
 * 
 * @param sockfile File for socket 
 * @param disURL dissected Url containing host and filepath 
 * @param dest given destination to write to (default is stdout) 
 */
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
    if((strcmp(tmp,"HTTP/1.1")) != 0){
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
/**
 * @brief main function 
 * 
 * @param argc argument count 
 * @param argv argument vector 
 * @return int 0 on success 1 on failure 
 */
int main(int argc, char *argv[])
{   
    programName = argv[0];
    Info info;
    // Synopsis: client [-p PORT] [ -o FILE | -d DIR ] URL
    argumentParsing(&info,argc,argv);
    DisURL dissectedURL;
    dissectURL(&dissectedURL,info.url, info.optDir);
    
    int sockfd;
    if((sockfd = setupSocket(dissectedURL.host, info.port)) == -1){
        fprintf(stderr,"failed at Socket setup error: %s in %s", strerror(errno), programName);
        exit(EXIT_FAILURE);
    }
    FILE *sockfile = fdopen(sockfd, "r+");
    
    char* file = NULL;


    if(info.file != NULL){
        file = info.file;
    }

    if(info.optDir == 1){
        file = (char*) malloc(strlen(info.dir) + strlen(dissectedURL.saveFile)+ 1);

        strcpy(file,info.dir);
        strcat(file,"/");
        strcat(file, dissectedURL.saveFile);
    }

    readFromSocket(sockfile,&dissectedURL,file);

    fclose(sockfile);
    if(info.dir != NULL) free(file);
    free(file);

    

}