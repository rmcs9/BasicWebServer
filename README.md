# BasicWebServer
Basic web server in C that takes an HTTP request for a file and sends back a HTTP response including the content of the file. 

## Methods:
- **void web_server(char *dir)**: <br>
opens up a new webserver at the current IP and listens for connections in a loop. If a new connection is found, a new thread is created to accept a request from the connected client. <br>
- **void *thread_caller(void *params)**: <br> 
a method called with a new thread that takes in a "struct connectParams". thread_caller waits to recieve a request from the client and forwards all request information to file_fetch_and_send() method <br>
- (**)int file_fetch_and_send(char* homedir, int connection, char *request)(**): <br>
method takes in the home directory, client connection file descriptor,
and request string and attempts to find the requested file in the home dir and send it to the client

