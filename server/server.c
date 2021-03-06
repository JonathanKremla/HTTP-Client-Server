/**
 * @file server.c
 * @author Jonathan Kremla 
 * @brief Server sending Data to client through socket
 * @date 2022-01-06
 */
#include <stdlib.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <errno.h>
#include <time.h>
#include <signal.h>

char *programName;

volatile __sig_atomic_t quit = 0;
/**
 * @brief struct for saving information about connection
 * 
 */
typedef struct Info
{
    char *port;
    char *file;
    char *doc_root;
} Info;
/**
 * @brief struct representing the header to send to a client
 * 
 */
typedef struct Header
{
    char* req_method; 
    char* path; 
    char* version; 
    char* empty; 
} header;
/**
 * @brief print usage message and exit with Exit Code 1 
 */
void usage()
{
    fprintf(stdout, "Usage in %s: server [-p PORT] [-i INDEX] DOC_ROOT", programName);
    exit(EXIT_FAILURE);
}
/**
 * @brief function for handling argumentParsing
 * 
 * @param inf Info struct saving information for our connection 
 * @param argc argument count 
 * @param argv argument vector 
 */
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
            usage();
        }
    }

    if (p_ind > 1 || i_ind > 1)
    {
        usage();
    }

    if ((i_ind * 2) + (p_ind * 2) != argc - 2)
    {
        usage();
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

/**
 * @brief fetch current Date and Time example: "Sun, 11 Nov 18 22:55:00 GMT"  
 * 
 * @return char* representing current Date and Time 
 */
char* getDate(){
    int dateSize = 100;
    char* date = malloc(dateSize);
    time_t t = time(NULL); 
    struct tm *ti = localtime(&t);
    
    strftime(date, dateSize, "%a, %d %b %y %T %Z", ti);
    return date;
}
/**
 * @brief Get the File Size of file 
 * 
 * @param file file name 
 * @return int file size 
 */
int getFileSize(char* file){
    struct stat sb;

    if(stat(file,&sb) == -1){
        return -1;
    }
    int size = sb.st_size;
    return size;
}
/**
 * @brief setup Socket for Communicating with client
 * 
 * @param port port to connect on 
 * @return int return -1 on failure fd of socket on success
 */
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
        fprintf(stderr, "at socket ");
        return -1;
    }
    int optval = 1;
    if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval) != 0){
        fprintf(stderr, "at setsockopt ");
        return -1;
        
    }
    if ((res = bind(sockfd, ai->ai_addr, ai->ai_addrlen)) < 0) 
    {
        fprintf(stderr, "at bind ");
        return -1;
    }
    int temp;
    if ((temp = listen(sockfd, 128)) == -1)
    {
        fprintf(stderr, "at listen ");
        return -1;
    }
    freeaddrinfo(ai);
    return sockfd;
}
/**
 * @brief extract the Header sent by client
 * 
 * @param sockfile socket file 
 * @return struct Header struct to save Header into 
 */
static struct Header extractHeader(FILE *sockfile){

    char* line;
    size_t len = 0;
    if(getline(&line,&len,sockfile) == -1){
        fprintf(stderr, "failed at reading request Header in %s",programName);
        fclose(sockfile);
        exit(EXIT_FAILURE);
    }
    struct Header head;
    char* request_header = malloc(strlen(line) + 1);
    request_header = strcpy(request_header,line);

    head.req_method = strtok(request_header, " ");
    head.path = strtok(NULL, " ");
    head.version = strtok(NULL, " ");
    head.empty = strtok(NULL, "\r\n");
    free(line);
    free(request_header);
    return head;
}
/**
 * @brief parsing recieved Header  
 * 
 * @param head Header struct, with received Header information 
 * @return int returns the correct message Code, which will be sent to client later on 
 */
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
/**
 * @brief reads until end of file, used to skip through entire file after reading header.
 * 
 * @param sockfile socket file to be read 
 */
void skipBody(FILE *sockfile){

    char* npt = NULL;
    size_t len_npt = 0;

    do{
        getline(&npt,&len_npt, sockfile);
    }
    while((strcmp(npt,"\r\n")) != 0);
    free(npt);

}
/**
 * @brief write content into socket file
 * 
 * @param sockfile socket file to be written in
 * @param content content to write 
 */
void writeContent(FILE *sockfile,FILE *content ){

    char* line = NULL;
    size_t line_length = 0;
    fflush(sockfile); 
    while((getline(&line, &line_length, content)) != -1){
        fprintf(sockfile,"%s",line);
    }    
    free(line);
}
/**
 * @brief signal handling function
 * 
 * @param signal which occured in process
 */
static void handle_signal(int signal){
    fprintf(stderr, "interrupted by Signal in %s ",programName);
    quit = 1;
}
/**
 * @brief check if char *s ends with "\", thus being a directory
 * 
 * @param s string to be checked 
 * @return true if s ends with "\" 
 * @return false if s does not end with "\" 
 */
static bool isDir(char *s)
{
    int size = strlen(s);
    
    if(s[size - 1] == '/')
        return true;
    
    return false;
}


/**
 * @brief this function represents the whole communication process between server and client,
 * running in a loop until interrupted.
 * 
 * @param sockfd socket file, communication file between server and client 
 * @param doc_root the root file of this server 
 * @param stdFile the standart file, if not specified otherwise "index.html" 
 */
void chat(int sockfd, char* doc_root, char* stdFile){

    //accept connection
    if ((sockfd = accept(sockfd, NULL, NULL)) < 0)
    {
        if(errno == EINTR){
            exit(EXIT_SUCCESS);
        }
        fprintf(stderr, "\nfailed at accept error: %s in %s ",strerror(errno),programName);
        exit(EXIT_FAILURE);
    }


    //open socketfile
    FILE *sockfile;
    if ((sockfile = fdopen(sockfd, "w+")) == NULL)
    {
        fprintf(stderr, "\nfailed at opening File error: %s in %s ",strerror(errno),programName);
        exit(EXIT_FAILURE);
    }

    //read request header
    struct Header request_header = {NULL,NULL, NULL, NULL};
    request_header = extractHeader(sockfile);

    //parse header, correct if 200
    int msgCode = 0;
    msgCode = parseHeader(&request_header);

    //skip rest of file
    skipBody(sockfile);

    //extract path of file to send
    char f_path[strlen(doc_root) + strlen(request_header.path) + strlen(stdFile) + 1];
    strcpy(f_path,doc_root);
    strcat(f_path, request_header.path);

    if(isDir(request_header.path)){
        strcat(f_path,stdFile);
    }

    //write header
    FILE *content = fopen(f_path, "r");
    if(content == NULL && msgCode == 200){
        msgCode = 404;
    }
    if(msgCode != 200){
        fprintf(sockfile,"HTTP/1.1 %d \r\nConnection: close\r\n\r\n", msgCode);
        fflush(sockfile);
    }
    else{
        int length;
        if((length = getFileSize(f_path)) == -1){
            fprintf(stderr, "file not found error: %s in %s ",strerror(errno), programName);
        }
        char* date = getDate();
        char* status = "OK";

        if(fprintf(sockfile, "HTTP/1.1 %d %s\r\nDate: %s\r\nContent-Length: %d\r\nConnection: close\r\n\r\n", msgCode, status, date, length) < 0){
            fprintf(stderr, "failed at print in %s", programName);
        }
        //write content
        writeContent(sockfile,content);
        free(date);
    }
    fclose(sockfile);
    fclose(content);

}



/**
 * @brief main function structuring program
 * 
 * @param argc argument count 
 * @param argv argument vector 
 * @return int returns 0 on Success, 1 on Failure 
 */
int main(int argc, char *argv[])
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    //signal handling
    sa.sa_handler = handle_signal;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    programName = argv[0];
    Info info = {NULL,NULL,NULL};
    argumentParsing(&info, argc, argv);
    int sockfd = setupSocket(info.port);
    if(sockfd < 0){
        fprintf(stderr, "\nfailed at socket setup, error: %s in %s\n", strerror(errno), programName);
        exit(EXIT_FAILURE);
    }
    while(quit == 0){
        chat(sockfd, info.doc_root, info.file);
    }
    close(sockfd);
    exit(EXIT_SUCCESS);
}