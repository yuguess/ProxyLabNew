/*
 * Name     : Dalong Cheng, Fan Xiang
 * Andrew ID: dalongc, fanx
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "csapp.h"

#define MAX_OBJECT_SIZE ((1 << 10) * 100) 
#define MAX_CACHE_SIZE (1 << 20)

int min(int a, int b) {
    return a > b ? b : a;
}

typedef struct Cache_Block Cache_Block;

struct Cache_Block{
    unsigned long key;
    char *content;
    size_t content_size;
    time_t time_stamp;
    Cache_Block *next_block;
};

typedef struct {
    size_t size; 
    Cache_Block *start; 
} Cache;

typedef struct {
    char request_str[MAXLINE]; 
    char host_str[MAXLINE];
} Request;

typedef struct {
    char header[MAXLINE];
    char *content;
    int content_length;
} Response;

typedef struct {
    int client_fd;
} Thread_Input;

Cache cache;

void proxy_error(char *msg) {
    fprintf(stderr, "%s: %s\n", msg, strerror(errno));
}

static inline void copy_single_line_str(rio_t *client_rio, char *buffer) {
    if (rio_readlineb(client_rio, buffer, MAXLINE) < 0) {
        proxy_error("copy_single_lienstr error");
    }
    buffer[strlen(buffer)] = '\0';
}

static inline void extract_hostname(char *src, char *desc) {
    int copy_length;
    copy_length = strlen(src + 6) - 2;  
    strncpy(desc, src + 6, copy_length); 
    desc[copy_length] = '\0';
}

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

void send_client(Response *response) {

}

int check_cache(Request *request, Response *response) {
    return 0;
}

static inline int extract_port_number(char *resquest_str) {
    return 80;
}

void forward_response(int client_fd, rio_t *server_rio, Response *response) {
    size_t n;
    int length = -1;
    char header_buffer[MAXLINE];
    char content_buffer[MAX_OBJECT_SIZE];
    int read_size;
    
    while ((n = rio_readlineb(server_rio, header_buffer, MAXLINE)) != 0) { 
        strcat(response->header, header_buffer); 
        /*
        if (rio_writen(client_fd, header_buffer, n) < 0) {
            proxy_error("rio_writen in forward_response header error");  
        }*/
        
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
    printf("response header: %s", response->header);
    #endif

    int sum = 0;
    while ((n = rio_readnb(server_rio, content_buffer, read_size)) != 0) { 
        if (rio_writen(client_fd, content_buffer, n) < 0) {
            proxy_error("rio_writen in forward_response content error");  
            Close(client_fd);
        }
        sum += n;
    }

    #ifdef DEBUG 
    printf("read byte size:%u\n", sum);
    #endif
   /* 
    if (sum <= MAX_OBJECT_SIZE) {
        response->content = Malloc(sizeof(char) * sum);
        response->content_length = sum;
    } else {
        response->content_length = sum;
    }*/
}

void forward_request(int client_fd, Request *request, Response *response) {
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

    server_fd = open_clientfd(hostname, port);

    rio_readinitb(&server_rio, server_fd);
    #ifdef DEBUG
    printf("request_str:%s", request->request_str); 
    #endif
    
    rio_writen(server_fd, request->request_str, strlen(request->request_str));
    rio_writen(server_fd, request->host_str, strlen(request->host_str));
    rio_writen(server_fd, "\r\n", strlen("\r\n"));
    
    forward_response(client_fd, &server_rio, response);
}

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

void save_to_cache(Response *response) {
    //lock !
    /*
    if (cache) {

    }*/

    //unlock !
}

void *request_handler(void *ptr) {
    int client_fd = ((Thread_Input*)ptr)->client_fd; 
    printf("client_fd:%d\n", client_fd);
    Request request;
    Response response;
    parse_request_header(client_fd, &request);
    modify_request_header(&request);
    
    if (check_cache(&request, &response)) {
        send_client(&response);
    } else {
        forward_request(client_fd, &request, &response);
        /*
        if (response.length <= MAX_OBJECT_SIZE)
            save_to_cache(&response);
            */
    }
    free(ptr);
    Close(client_fd);
    #ifdef DEBUG
    printf("connection close\n\n");
    #endif
    return NULL; 
}

void scheduler(int client_fd) {
    pthread_t tid;

    Thread_Input *thread_input = (Thread_Input*)malloc(sizeof(Thread_Input));
    thread_input->client_fd = client_fd;

    Pthread_create(&tid, NULL, request_handler, thread_input);
    Pthread_detach(tid);
}

void init_cache() {
    cache.start = NULL;
    cache.size = 0;
}

void clean_cache() {

}

int main (int argc, char *argv []) {
    int listen_fd, port;
    int client_fd;
    socklen_t socket_length;
    struct sockaddr_in client_socket;

    init_cache();
    if (argc != 2) {
	    fprintf(stderr, "usage: %s <port>\n", argv[0]);
	    exit(0);
    }

    port = atoi(argv[1]);
    listen_fd = open_listenfd(port);
    socket_length = sizeof(client_socket);

    while (1) {
        client_fd = Accept(listen_fd, (SA*)&client_socket, &socket_length);   
        scheduler(client_fd);
    }
    clean_cache();
}
