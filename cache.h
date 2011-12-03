/*
 * File     : cache.h
 * Author   : Dalong Cheng, Fan Xiang
 * Andrew ID: dalongc, fanx
 */
#ifndef __CACHE_H__
#define __CACHE_H__

#include "csapp.h"

#define MAX_OBJECT_SIZE ((1 << 10) * 100) 
#define MAX_CACHE_SIZE (1 << 20)

/* reqeust struct, wrapper for client request */
typedef struct Cache_Block Cache_Block;
typedef struct {
    char request_str[MAXLINE]; 
    char host_str[MAXLINE];
} Request;

/*  response struct, wrapper for server's response */
typedef struct {
    char header[MAXLINE];
    char *content;
    int content_size;
} Response;

struct Cache_Block{
    unsigned long key;
    char *content;
    int content_size;
    time_t time_stamp;
    Cache_Block *next_block;
    Cache_Block *pre_block;
};

typedef struct {
    int size; 
    Cache_Block *head; 
    Cache_Block *tail;
} Cache;

/* Global variables */
Cache cache; // cache for proxy
pthread_mutex_t mutex_lock; // lock when accessing cache

/* Cache related function prototypes */
Cache_Block *is_in_cache(unsigned long);
Cache_Block *build_cache_block(Request *, Response *);
void add_cache_block(Cache_Block *);
void delete_link(Cache_Block *);
void free_cache_block(Cache_Block *);
void delete_cache_block(Cache_Block *);
int check_cache(Request *, Response *); 
void save_to_cache(Request *, Response *);
void init_cache();
void clean_cache();

#endif
