#include <stdio.h>
#include "csapp.h"
#include "thread_pool.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE  1049000
#define MAX_OBJECT_SIZE 102400
#define MAX_CACHE_NUM   10
#define PORT_MAX        10
#define HEAD_NAME_MAX   32
#define HEAD_VALUE_MAX  64

/* Request Structure */
typedef struct {
    char host[HOST_NAME_MAX];
    char port[PORT_MAX];
    char path[MAXLINE];
    char uri[MAXLINE];
} ReqLine;

typedef struct {
    char name[HEAD_NAME_MAX];
    char value[HEAD_VALUE_MAX];
} ReqHeader;

/* Cache Structure */
typedef struct {
    char *uri;
    char *value;
    size_t len;
} Cache;

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";

Cache cache[MAX_CACHE_NUM];
int cache_cnt = 0;
sem_t empty, full;

void *doit(void* fd);
void parse_request(int fd, ReqHeader headers[], ReqLine* request_line, int *num_hds);
void read_requesthdrs(rio_t *rp);
int parse_uri(char *uri, char *filename, char *cgiargs);
void serve_static(int fd, char *filename, int filesize);
void get_filetype(char *filename, char *filetype);
void serve_dynamic(int fd, char *filename, char *cgiargs);
void clienterror(int fd, char *cause, char *errnum, 
		 char *shortmsg, char *longmsg);
void proxy_route(int fd, ReqHeader headers[], ReqLine* request_line, int num_hds);

void init_cache();
int cache_get(char *uri);
int cache_write(char *uri, char *object);

int main(int argc, char **argv)
{
    int listenfd, *connfd;
    char hostname[MAXLINE], port[MAXLINE];
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;

    /* Check command line args */
    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }

    init_cache();
    init_thread_pool(0);
    listenfd = Open_listenfd(argv[1]);
    while (1) {
        clientlen = sizeof(clientaddr);
        connfd = Malloc(sizeof(int));
        *connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen); 
            Getnameinfo((SA *) &clientaddr, clientlen, hostname, MAXLINE, 
                        port, MAXLINE, 0);
        printf("Accepted connection from (%s, %s)\n", hostname, port);
        thread_pool_add_task(doit, connfd);
    }
    return 0;
}

void proxy_route(int fd, ReqHeader headers[], ReqLine* line, int num_hds) {
    int total_size = 0, n, clientfd;
    char *buf_head, buf[MAXLINE], object_buf[MAX_OBJECT_SIZE];
    rio_t rio;

    clientfd = Open_clientfd(line->host, line->port);
    sprintf(buf, "GET %s HTTP/1.0 \r\n", line->path);
    
    Rio_readinitb(&rio, clientfd);
    while((n = Rio_readlineb(&rio, buf, MAXLINE))) {
        Rio_writen(fd, buf, n);
        strcpy(object_buf + total_size, buf);
        total_size += n;
    }

    if (total_size < MAX_OBJECT_SIZE) {
        cache_write(line->uri, object_buf);
    }

    Close(clientfd);
}

void *doit(void* clientfd) {
    int fd = *((int*)clientfd);
    Free(clientfd);
    ReqHeader headers[20];
    ReqLine request_line;

    int num_hds;
    parse_request(fd, headers, &request_line, &num_hds);

    int cache_idx = -1;
    if ((cache_idx = cache_get(request_line.uri)) != -1) {
        Rio_writen(fd, cache[cache_idx].value, cache[cache_idx].len);
        return;
    }
    
    // 没找到缓存
    proxy_route(fd, headers, &request_line, num_hds);

    Close(fd);
}
