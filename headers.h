#include<sys/socket.h>
#include<arpa/inet.h>
#include<string.h>
#include<fcntl.h>
#include<unistd.h>
#include<netinet/in.h>
#include<stdlib.h>
#include<stdio.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<pthread.h>
#include<semaphore.h>
#include<time.h>
#include<error.h>
#include<sys/wait.h>
#include<netdb.h>

#define MAX_LEN 4*1024						// maximum allowed size of request/response
#define MAX_CLIENTS 200						// maximum number of client requests handled at a time
#define MAX_CACHE_SIZE 200*(1<<20)			// size of cache 2000mb
#define MAX_CACHE_ELEMENT_SIZE 10*(1<<20)	// max size of an element in cache 100mb

#define HEADER_LEN	10


/*
lru_cache_element objects will store the responses of request that were previously made.
The data that we intend to store is the url of request, its size and the fetched response for the same
*/

typedef struct lru_cache_element lru_cache_element; 
struct lru_cache_element{
    char* url;                  //stores the requested url
    char* response;             //stores the response
    int length;                 //stores the length of the response
    lru_cache_element* next;    //pointer to the next cache_element
    lru_cache_element* prev;    //pointer to the prev cache_element
};

typedef struct Request HTTP_Request;
struct Request{
	size_t buflen;
	size_t headersused;
	size_t headerslen;
	char* method;
	char* protocol;
	char* host;
	char* port;
	char* path;
	char* version;
	char* buf;
	struct Headers* headers;
};

struct Headers{
	size_t keylen;
	size_t valuelen;
	char* key;
	char* value;
};


extern sem_t semaphore;                //used to put waiting threads to sleep and wake up
extern pthread_mutex_t mutex_lock;     //used for locking the cache

extern lru_cache_element* head;        //head pointer of the lru cache
extern lru_cache_element* tail;        //tail pointer of the lru cache

// functions in server.c
void send_internal_error(int client);
int handle_client_request(int client,HTTP_Request* request,char* client_request);
int verify_HTTP_version(char* version_string);
void send_internal_error(int client_fd);
int handle_client_request(int client_connection, HTTP_Request* request, char* client_request);
int connect_to_server(char* client_host, int server_port);

// functions in parse.c
HTTP_Request* create_request();
void destroy_request(HTTP_Request* request);
void create_headers_for_request(HTTP_Request* temp);
int parse_client_request(char* client_request, HTTP_Request* request);
int parse_headers_for_client_request(HTTP_Request* request, char* curr_header);
int remove_header(HTTP_Request* request, const char* key);
struct Headers* get_header_from_http_request(HTTP_Request* request,const char* key);
void destroy_headers(HTTP_Request* request);
void destroy_single_header(struct Headers* header);
int reconstruct_response_from_headers(HTTP_Request* request, char* buffer, size_t buffer_len);
int set_header_for_http_request(HTTP_Request* request, char* key, char* value);

// functions in lru_cache.c
void init_cache();
void destroy_cache();
lru_cache_element* find_in_cache(char* buffer);
void remove_last_element();
void add_element_to_begin(lru_cache_element* element);
int add_cache_element(char* response, int length, char* url);



