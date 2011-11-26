/*
 * Name:   Dalong Cheng, Fan Xiang
 * Andrew: dalongc, fanx
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "csapp.h"

typedef struct {
    int client_fd; 
    int port;
    char hostname[MAXLINE];
    char method[MAXLINE];
    char version[MAXLINE];
    char uri[MAXLINE];
    char buffer[MAXLINE];
    char request_file[MAXLINE];
} Request_info;

void client_error(int fd, char *cause, char *errnum, 
		 char *shortmsg, char *longmsg) 
{
    char buf[MAXLINE], body[MAXBUF];

    /* Build the HTTP response body */
    sprintf(body, "<html><title>Proxy Error</title>");
    sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
    sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
    sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
    sprintf(body, "%s<hr><em>Proxy only implement GET method!</em>\r\n", body);

    /* Print the HTTP response */
    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
    Rio_writen(fd, buf, strlen(buf));
    Rio_writen(fd, body, strlen(body));
}

/*
 * request_handler - build up request to dest server in a new thread
 */
void *request_handler(void *ptr) {
    int server_fd;
    rio_t server_rio;

    Request_info *request_info = (Request_info*)ptr;
    
    server_fd = open_clientfd(request_info->hostname, request_info->port);
    printf("connect to %s:%d\n", request_info->hostname, request_info->port);
    rio_readinitb(&server_rio, server_fd);
    rio_writen(server_fd, request_info->buffer, strlen(request_info->buffer));
    printf("send to server %s \n", request_info->buffer);
    while ((rio_readlineb(&server_rio, request_info->buffer, MAXLINE)) != 0) { 
        rio_writen(request_info->client_fd, request_info->buffer, 
                strlen(request_info->buffer));
    }
    Close(server_fd);
    free(ptr);
    return NULL;
}

/*
 * scheduler - schedule a thread for each request 
 */
void scheduler(Request_info *request_info) {
    pthread_t tid;

    Pthread_create(&tid, NULL, request_handler, request_info);
    Pthread_join(tid, NULL);
}

/*
 * uri_parser - extract hostname and port number(if specified)
 */
void uri_parser(char *str, Request_info *request_info) {
    int scheme_length = 7;
    int port_begin;
    int hostname_length;
    int request_file_span;
    char *temp_str;
    char file[MAXLINE];
    char buffer[MAXLINE];
    strncpy(file, "/", 1);
    file[1] = '\0';
    temp_str = strstr(str, "http://");
    if (temp_str == NULL) {
        scheme_length = 0;
        temp_str = str;
    }
    
    port_begin = strcspn(temp_str + scheme_length, ":");
    if (strlen(temp_str + scheme_length) == port_begin) {
        hostname_length = strcspn(temp_str + scheme_length, "/");

        strncpy(request_info->hostname, 
                temp_str + scheme_length, hostname_length);
        request_info->hostname[hostname_length] = '\0';

        request_file_span = scheme_length + hostname_length; 
        if (strlen(temp_str) != request_file_span) {
            strncpy(file, temp_str + request_file_span, 
                    strlen(temp_str) - request_file_span);
            file[strlen(temp_str) - request_file_span] = '\0';
        }
    } else {
        hostname_length = port_begin;
        strncpy(request_info->hostname, 
                temp_str + scheme_length, hostname_length);
        request_info->hostname[hostname_length] = '\0';

        sscanf(temp_str + scheme_length + port_begin + 1, "%d", 
                &request_info->port); 

        hostname_length = strcspn(temp_str + scheme_length, "/");
        request_file_span = scheme_length + hostname_length; 
        if (strlen(temp_str) != request_file_span) {
            strncpy(file, temp_str + request_file_span, 
                    strlen(temp_str) - request_file_span);
            file[strlen(temp_str) - request_file_span] = '\0';
        }
    }

    sprintf(buffer, "GET %s %s \n\r\n", file, request_info->version);
    strcpy(request_info->buffer, buffer);
}

/*
 * init_request_info - initliaze and build up request info struct 
 */
Request_info * init_request_info(int client_fd, char *buffer) {
    Request_info *request_info;

    request_info = (Request_info*)Malloc(sizeof(Request_info));
    request_info->client_fd = client_fd;
    request_info->port = 80;
    strncpy(request_info->buffer, buffer, MAXLINE);
    sscanf(buffer, "%s %s %s", request_info->method, 
            request_info->uri, request_info->version);
    uri_parser(request_info->uri, request_info);
    if (strcasecmp(request_info->method, "GET")) { 
       client_error(client_fd, request_info->method, "501", "Not Implemented",
                " Not implement this method");
        return NULL;
    }
    return request_info;
}

/*
 * parse_request - wrapper like function for scheduler  
 */
void parse_request(int client_fd) {
    size_t n;
    char buffer[MAXLINE];
    rio_t client_rio;
    Request_info *request_info = NULL;
    
    rio_readinitb(&client_rio, client_fd);
    while ((n = rio_readlineb(&client_rio, buffer, MAXLINE)) != 0) { 
        printf("server received from client %lu bytes, %s", n, buffer);
        if ((request_info = init_request_info(client_fd, buffer)) != NULL)
            scheduler(request_info);
    }
}

int main (int argc, char *argv []) {
    int listenfd, connfd, port;
    socklen_t socket_length;
    char * ip_str;
    struct sockaddr_in client_socket;
    struct hostent *host_info;

    if (argc != 2) {
	    fprintf(stderr, "usage: %s <port>\n", argv[0]);
	    exit(0);
    }
    port = atoi(argv[1]);
    listenfd = Open_listenfd(port);

    while (1) {
        socket_length = sizeof(client_socket);
	    connfd = Accept(listenfd, (SA*)&client_socket, &socket_length);

        host_info = Gethostbyaddr((const char*)&client_socket.sin_addr.s_addr,
                sizeof(client_socket.sin_addr.s_addr), AF_INET);
        ip_str = inet_ntoa(client_socket.sin_addr);

        printf("server establish connection with %s(%s)\n",
                host_info->h_name, ip_str);

        parse_request(connfd);
        Close(connfd); 
    }
    exit(0);
}
