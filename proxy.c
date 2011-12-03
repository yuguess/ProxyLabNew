/*
 * File     : proxy.c
 * Author   : Dalong Cheng, Fan Xiang
 * Andrew ID: dalongc, fxiang
 * 
 * General Code Structure
 * 1. Use producer and consumer model to handle client request 
 * 2. Use link list to cache request file's crc32 hash value 
 * 3. Use LRU policy to evivt cache block
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "csapp.h"
#include "sbuf.h"
#include "crc32.h"
#include "cache.h"

/* Global variables */
sbuf_t sbuf; /* shared buffer of connected descriptors */

/* Internal function prototypes */
static inline int min(int, int);
static inline void copy_single_line_str(rio_t *, char *);
static inline void extract_hostname(char *, char *);
static inline int extract_port_number(char *);
void modify_request_header(Request *);
void send_client(int, Response *);
void forward_response(int, int, Response *);
void proxy_error(char *);
int forward_request(int, Request *, Response *);
void parse_request_header(int, Request *);
void *request_handler(int);
void *worker_thread(void *);

/* Macto definition*/
#define NTHREADS  40
#define SBUFSIZE  20
#define state_ofs 9
int main (int argc, char *argv []) {
    int listen_fd, port;
    int client_fd;
    int i;
    pthread_t tid;
    
    socklen_t socket_length;
    struct sockaddr_in client_socket;

    pthread_mutex_init(&mutex_lock, NULL);
    init_cache();

    if (argc != 2) {
	    fprintf(stderr, "usage: %s <port>\n", argv[0]);
	    exit(0);
    }
    port = atoi(argv[1]);   
    socket_length = sizeof(client_socket);
    /* ignore SIGPIPE */
    Signal(SIGPIPE, SIG_IGN);

    /* initialize worker thread, then initialize buffer
     * for producer-consumer model */
    sbuf_init(&sbuf, SBUFSIZE);
    for (i = 0; i < NTHREADS; i++)
        Pthread_create(&tid, NULL, worker_thread, NULL);

    listen_fd = Open_listenfd(port);

    while (1) {
        client_fd = Accept(listen_fd, (SA*)&client_socket, &socket_length);
        sbuf_insert(&sbuf, client_fd);
    }
    pthread_mutex_destroy(&mutex_lock);
    clean_cache();
}

/* 
 * min - return min value of two value 
 */
static inline int min(int a, int b) {
    return a > b ? b : a;
}

/* 
 * copy_single_line_str - store a line of str 
 */
static inline void copy_single_line_str(rio_t *client_rio, char *buffer) {
    if (rio_readlineb(client_rio, buffer, MAXLINE) < 0) {
        proxy_error("copy_single_lienstr error");
    }
    buffer[strlen(buffer)] = '\0';
}

/* 
 * extract_hostname - extract hostname from string 
 */
static inline void extract_hostname(char *src, char *desc) {
    int copy_length;
    copy_length = strlen(src + 6) - 2;  
    strncpy(desc, src + 6, copy_length); 
    desc[copy_length] = '\0';
}

/* 
 * extract_port_number - extract port from input str 
 */
static inline int extract_port_number(char *resquest_str) {
    return 80;
}

/* 
 * modify_request_header - modify input str into HTTP/1.0 request 
 */
void modify_request_header(Request *request) {
    char *str;
    if ((str = strstr(request->request_str, "HTTP/1.1")) != NULL) {
        strncpy(str, "HTTP/1.0", 8); 
    } else {
        fprintf(stderr, "Warning: HTTP/1.1 not found in request\n");
    }
    #ifdef DEBUG
    printf("modify request str:%s", request->request_str);
    #endif
}

/* 
 * send_client - send resposne content to client
 */
void send_client(int client_fd, Response *response) {
    #ifdef DEBUG
    printf("enter send_client \n");
    #endif
    if (response != NULL) {
        if (response->content != NULL) {
            if (rio_writen(client_fd, response->content, 
                        response->content_size) < 0) {
                proxy_error("rio_writen in send_client error");  
            }
        } else {
            fprintf(stderr, "error in send_client, content is NULL\n"); 
            abort();
        }
    } else {
        fprintf(stderr, "error in send_client, response is NULL\n"); 
        abort();
    }
}

/* 
 * forward_response - send request to server, then store resposne content
 * in cache if necessary 
 */
void forward_response(int client_fd, int server_fd, Response *response) {
    #ifdef DEBUG
    printf("enter forward_response\n");
    #endif
    size_t n;
    int length = -1;
    char header_buffer[MAXLINE];
    char content_buffer[MAX_OBJECT_SIZE];
    int read_size;
    rio_t   server_rio;

    rio_readinitb(&server_rio, server_fd);
    while ((n = rio_readlineb(&server_rio, header_buffer, MAXLINE)) != 0) { 
        strcat(response->header, header_buffer); 
        
        if (rio_writen(client_fd, header_buffer, n) < 0) {
            proxy_error("rio_writen in forward_response header error");  
        }
        
        if (strstr(header_buffer, "Content-Length: ")) {
            sscanf(header_buffer + 16, "%d", &length);
        }
        if (!strcmp(header_buffer, "\r\n")) {
            break;
        }
    }

    if (length == -1)
        read_size = MAX_OBJECT_SIZE;
    else 
        read_size = min(length, MAX_OBJECT_SIZE);
    
    #ifdef DEBUG
    printf("finish response header\n");
    #endif

    int sum = 0;
    while ((n = rio_readnb(&server_rio, content_buffer, read_size)) != 0) { 
        if (rio_writen(client_fd, content_buffer, n) < 0) {
            proxy_error("rio_writen in forward_response content error");  
            close(client_fd);
        }
        sum += n;
    }

    #ifdef DEBUG 
    printf("read byte size:%u\n", sum);
    #endif
    
    if (sum <= MAX_OBJECT_SIZE) {
        response->content = Malloc(sizeof(char) * sum);
        memcpy(response->content, content_buffer, sum * sizeof(char)); 
        response->content_size = sum;
    } else {
        response->content_size = sum;
    }
    #ifdef DEBUG
    printf("leave forward_response\n");
    #endif
}

/* 
 * proxy_error - proxy error wrapper function 
 */
void proxy_error(char *msg) {
    fprintf(stderr, "%s: %s\n", msg, strerror(errno));
}

/* 
 * forward_request - send request from client to its destination server 
 */
int forward_request(int client_fd, Request *request, Response *response) {
    rio_t   server_rio;
    int server_fd;
    char hostname[MAXLINE];
    int  port = 80;

    port = extract_port_number(request->request_str);
    extract_hostname(request->host_str, hostname);

    #ifdef DEBUG
    printf("hostname:%s\n", hostname);
    printf("port:%d\n", port);
    #endif
    if ((server_fd = open_clientfd(hostname, port)) < 0) {
       fprintf(stderr, "Warning connection refused !\n"); 
       return -1;
    }

    rio_readinitb(&server_rio, server_fd);
    #ifdef DEBUG
    printf("request_str:%s", request->request_str); 
    #endif
    
    rio_writen(server_fd, request->request_str, strlen(request->request_str));
    rio_writen(server_fd, request->host_str, strlen(request->host_str));
    rio_writen(server_fd, "\r\n", strlen("\r\n"));
    
    forward_response(client_fd, server_fd, response);
    close(server_fd);
    return 1;
}

/* 
 * parse_request_header - parse request header extract useful information 
 */
void parse_request_header(int client_fd, Request *request) {
    size_t  n;
    rio_t   client_rio;
    char buffer[MAXLINE];

    rio_readinitb(&client_rio, client_fd);
    copy_single_line_str(&client_rio, request->request_str);
    copy_single_line_str(&client_rio, request->host_str);

    #ifdef DEBUG
    printf("request str:%s", request->request_str);
    printf("host str: %s", request->host_str);
    #endif
    
    while ((n = rio_readlineb(&client_rio, buffer, MAXLINE)) != 0) { 
        #ifdef DEBUG
        printf("%s", buffer); 
        #endif
        if (!strcmp(buffer, "\r\n")) {
            break;
        }
    }
}

/* 
 * request_handler - general function to handler each client request 
 */
void *request_handler(int client_fd) {
    #ifdef DEBUG
    printf("enter request_handler\n");
    #endif

    Request request;
    Response response;
    parse_request_header(client_fd, &request);
    modify_request_header(&request);
    
    if (check_cache(&request, &response)) {
        send_client(client_fd, &response);
    } else {
        if (forward_request(client_fd, &request, &response) < 0) {
            close(client_fd);
            return NULL;
        } else {
            /* save to cache if status code 2XX and < max size */
            if (response.content_size <= MAX_OBJECT_SIZE && 
                response.header[state_ofs]=='2')
                save_to_cache(&request, &response);
        }
    }
    close(client_fd);
    #ifdef DEBUG
    printf("connection close\n\n");
    printf("leave request_handler\n");
    #endif
    return NULL; 
}

/* 
 * worker_thread - thread that initialized first, then invoke when 
 * reqeust comming in 
 */
void *worker_thread(void *vargp) {  
    Pthread_detach(pthread_self()); 
    while (1) { 
	    int client_fd = sbuf_remove(&sbuf);
        request_handler(client_fd);
        printf("client_fd:%d\n", client_fd);
    }
}
