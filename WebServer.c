#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>
#include <netdb.h>
#include <sys/stat.h>
#include <sys/file.h>


//method takes in the home directory, client connection file descriptor,
//and request string and attempts to find the requested file
//in the home dir and send it to the client
int file_fetch_and_send(char* homedir, int connection, char *request){
    //strings for the GET and requested file path
    char *getString, *filepath;
    //obtain GET
    getString = strtok(request, " ");
    //obtain requested file path
    filepath = strtok(NULL, " ");

    //concatenate homedir onto the requested file path
    char targetdir[strlen(homedir) + strlen(filepath)];
    strcpy(targetdir, homedir);
    strcat(targetdir, filepath);

    //string for file not found case
    char FILE_NOT_FOUND_MESSAGE[27] = "HTTP/1.0 404 File Not Found";
    //stat struct for requested file
    struct stat target_file_stats;
    //obtain stats for the requested file
    if((stat(targetdir, &target_file_stats)) != 0){
        send(connection, FILE_NOT_FOUND_MESSAGE, strlen(FILE_NOT_FOUND_MESSAGE), 0);
        return 1;
    }
    //checks to see if the requested file is a regular file
    if(S_ISREG(target_file_stats.st_mode) == 0){
        send(connection, FILE_NOT_FOUND_MESSAGE, strlen(FILE_NOT_FOUND_MESSAGE), 0);
        return 1;
    }



    //file descriptor for the target file
    int target_file_desc;
    //string for the content of the file
    char target_file_content[300];
    //open target file for reading
    if((target_file_desc = open(targetdir, O_RDONLY, S_IROTH)) != -1){
        //read from file
        int bytes_read = read(target_file_desc, target_file_content, 100);
        char new_content[100];
        //concat new lines onto target content
        while(bytes_read > 0){
            strcat(target_file_content, new_content);
            strcat(target_file_content, "\n");
            bytes_read = read(target_file_desc, new_content, 100);
        }
        strcat(target_file_content, "\0");
        close(target_file_desc);
    }
    else{
        char errormsg[42] = "HTTP/1.0 404 Failed to open requested file";
        send(connection, errormsg, strlen(errormsg), 0);
        return 1;
    }

    //following lines create the success message string to be sent to client
    char success_ln1[16] = "HTTP/1.0 200 OK\n";
    long contentlength = target_file_stats.st_size;
    char str[20];
    sprintf(str, "%ld", contentlength);
    char success_ln2[30] = "Content Length: ";
    strcat(success_ln2, str);
    strcat(success_ln2, "\n\n\n");

    char* success_message = malloc(strlen(success_ln1) + strlen(success_ln2) + strlen(target_file_content));

    strcat(success_message, success_ln1);
    strcat(success_message, success_ln2);
    strcat(success_message, target_file_content);

    //send success message with file content back to client
    send(connection, success_message, strlen(success_message), 0);
    return 0;
}

//struct for necessary information for file_fetch_and_send() being passed into the new thread
struct connectParams{
    int connection_fd;
    char *homedir;
};
//method being called for the new thread
void *thread_caller(void *params){
    struct connectParams *thread_params = (struct connectParams*)params;
    //receive new message from the client
    char buffer[1024];
    if ((recv(thread_params->connection_fd, buffer, 1024, 0)) <= 0) {
        fprintf(stderr, "web server failed to recieve incoming message from client");
    }
    else{
        file_fetch_and_send(thread_params->homedir, thread_params->connection_fd, buffer);
    }
    pthread_exit(NULL);
}

void web_server(char *dir){
    //the file descriptor for this web server that will be listening for connections
    int webserver_socketfd;

    //opening a new socket for the web server
    if((webserver_socketfd = socket(AF_INET, SOCK_STREAM, 0)) == 0){
        fprintf(stderr, "error opening web server socket");
        exit(1);
    }
    //new address info struct created
    //one for the hints of what connection we
    struct addrinfo hints, *results;
    //setting up address info hints
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_flags = AI_PASSIVE;
    hints.ai_socktype = SOCK_STREAM;

    getaddrinfo(NULL, "8000", &hints, &results);

    //bind the socket to the server address
    if((bind(webserver_socketfd, results->ai_addr, results->ai_addrlen)) < 0){
        fprintf(stderr, "failed to bind web server socket to host address");
        exit(1);
    }
    //open the server address for listening
    if(listen(webserver_socketfd, 5) < 0){
        fprintf(stderr, "web server failed to listen for requests");
        exit(1);
    }

    int new_socket_fd;
    struct sockaddr_in *clientaddr;
    struct socklen_t *client_sock_len;
    while(1) {
        //accept incoming connection
        if ((new_socket_fd = accept(webserver_socketfd, clientaddr, client_sock_len)) < 0) {
            fprintf(stderr, "web server failed to accept incoming connection");
            exit(1);
        }

        printf("client connected to server\n");


        //create new struct to be passed into the new thread
        struct connectParams *params = malloc(sizeof(new_socket_fd));
        params->connection_fd = new_socket_fd;
        params->homedir = dir;

        //create new thread for file operation
        pthread_t id;
        pthread_create(&id, NULL, thread_caller, params);
    }
}

int main(int argc, char** argv) {
    if(argc > 1) {
        web_server(argv[1]);
    }
    else{
        fprintf(stderr, "web server requires a directory argument to launch");
        exit(1);
    }
}