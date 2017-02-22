#include "csapp.h"
#include <stdio.h>
#include "cache.h"

/**
 * proxy.c
 * This file is private protected for the class 15-513 Intoduction to computer
 * system, assignment 7 proxy lab.
 * proxy.c is a simplified simulation for proxy, it parse information safely
 * between client and server in multi-threads, and store and access data in
 * list.
 * @Author: Hao Xu
 * @Andrew id: haox1
 */

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";
static const char *con_hdr = "Connection: close\r\n";
static const char *Proxy_con_hdr = "Proxy-Connection: close\r\n";
static char *version_default = "Http/1.0\r\n";
static void doit(int fd);
static void writeHeader(char *request, char *host);
void writeToClient(char *request, char *host, char *port, int client_fd);
static int parse_uri(char *uri, char *host, char *port_server, char *path);
void getNewPath(char *path, char *uri_pos, char *uri_pos_new, char *newPath);
static void clienterror(int fd, char *cause, char *errnum, 
         char *shortmsg, char *longmsg);
static void *thread(void *vargp);
static int totalsize = 0;
sem_t mutex;

int main(int argc, char **argv) {
    int listenfd, *connfd;
    char hostname[MAXLINE], port[MAXLINE];
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    pthread_t tid;
    /* Check command line agrs */
    if (argc != 2) {
        fprintf(stderr, "usage: %s <port> \n", argv[0]);
        exit(1);
    }

    listenfd = Open_listenfd(argv[1]);

    /* semaphore mutex initialization */
    sem_init(&mutex, 0, 1);

    while(1) {
        clientlen = sizeof(clientaddr);
        connfd = Malloc(sizeof(int));
        *connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
        Getnameinfo((SA *) &clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
        printf("Accepted connection from (%s, %s)\n", hostname, port);

        Pthread_create(&tid, NULL, thread, connfd);

    }
    /* free all caches in list before proxy is killed. */
    freeCache();
    return 0;
}

/* Thread routine */
void *thread(void *vargp) {

    int myid = *((int *) vargp);
    free(vargp);
    Pthread_detach(pthread_self());

    doit(myid);
    Close(myid);
    return NULL;

}

/*
 * doit - handle one HTTP request/response transaction
 */
void doit(int client_fd) {

    char client_buf[MAXLINE];
    char method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char host[MAXLINE], path[MAXLINE], request[MAXLINE], port_server[MAXLINE];

    rio_t client_rio;

    /* Read request line and headers */
    Rio_readinitb(&client_rio, client_fd);
    if (!Rio_readlineb(&client_rio, client_buf, MAXLINE))
        return;
    sscanf(client_buf, "%s %s %s", method, uri, version);
    if (strcasecmp(method, "GET")) {
        clienterror(client_fd, method, "501", "Not Implemented",
            "proxy does not implement this method");
        return;
    }

    /* Parse URI from GET request */
    if (!parse_uri(uri, host, port_server, path)) {
        printf("There is something wrong when parsing uri\n");
    }

    /* create new request */
    sprintf(request, "%s %s %s", method, path, version_default);

    /* Deal with http request header */
    writeHeader(request, host);
    writeToClient(request, host, port_server, client_fd);
}

/**
 * This function is used to return the content from server to client, it has to
 * forward the file from cache if it is reloading the same page. And evict the
 * cache if the total cache size is beyond the whole size available, and simply
 * discard it if the content size iss beyond the acceptable object size.
 */
void writeToClient(char *request, char *host, char *port_server, int client_fd) {
    char  server_buf[MAX_OBJECT_SIZE], bufNew[MAX_OBJECT_SIZE];
    rio_t server_rio;
    int server_fd;
    int n, size;
    size = 0;
    n = 0;
    char *content;
    /* mutually lock and unclock when accessing cache in list to protect
     * information in multithreads.
     */
    P(&mutex);


    if ((content = findCache(request)) != NULL) { // cache hit

        size_t length_data = strlen(content);

        Rio_writen(client_fd, content, length_data);
        V(&mutex);        

        length_data = 0;
    } else { // cache miss
        server_fd = Open_clientfd(host, port_server);

        /* associate the discriptor server_fd with buffer server_rio */
        Rio_readinitb(&server_rio, server_fd);

        Rio_writen(server_fd, request, strlen(request));
        V(&mutex);
        /* read from file server_rio to buffer server_buf */
        while ((n = Rio_readnb(&server_rio, server_buf, MAX_OBJECT_SIZE)) > 0) {
            P(&mutex);
            size = size + n; 
            if (size < MAX_OBJECT_SIZE) {
                memcpy(bufNew, server_buf, n);
            }

            Rio_writen(client_fd, server_buf, n);
            V(&mutex);
            memset(server_buf, 0, sizeof(server_buf));
        }
        if (size <= MAX_OBJECT_SIZE) { // new buf size is acceptable for cache
            P(&mutex);
            totalsize += size;
            if (totalsize <= MAX_CACHE_SIZE) { // cache if cache is not full
                addFirst(request, bufNew);
            } else {
                totalsize = totalsize - evictCache(request, bufNew);
            }
            V(&mutex);

        }
        Close(server_fd);
    }   
    Free(content);
}

/**
 * write the header into request.
 */
static void writeHeader(char *request, char *host) {
    char *hosthdr;
    hosthdr = Malloc(MAXLINE);
    sprintf(hosthdr, "Host: %s\r\n", host);
    strcat(request, hosthdr);
    strcat(request, user_agent_hdr);
    strcat(request, con_hdr);
    strcat(request, Proxy_con_hdr);
    strcat(request, "\r\n");

    Free(hosthdr);
   
    return;
}

/**
 * split the uri into host, port and path without host or port.
 *
 */
int parse_uri(char *uri, char *host, char *port_server, char *path) {
    char *uri_pos;
    int port = 80;
    uri_pos = strstr(uri, "http://") + sizeof("http://") - 1;

    while (*uri_pos != '/' && *uri_pos != ':') {
        *host = *uri_pos;
        uri_pos++;
        host++;
    }

    if (*uri_pos == ':') {

        uri_pos++;
        sscanf(uri_pos, "%d", &port);

    }

    sprintf(port_server, "%d", port);

    uri_pos = strchr(uri_pos, '/');
    strcat(path, uri_pos);

    char *uri_pos_new;
    if ((uri_pos_new = strstr(path, "/r/n")) == 0) {
        return 1;
    }

    char newPath[MAXLINE];

    getNewPath(path, uri_pos, uri_pos_new, newPath);

    strcpy(path, newPath);

    Free(uri_pos);
    Free(uri_pos_new);
    return 1;

}

/**
 * get new path if the path ends with "/r/n"
 */
void getNewPath(char *path, char *uri_pos, char *uri_pos_new, char *newPath) {
    while (strcmp(uri_pos, uri_pos_new)) {
        *newPath = *uri_pos;
        uri_pos++;
        newPath++;
    }
}
/**
 * client error function is used to check if the client error exists
 */
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg) {
    char buf[MAXLINE], body[MAXLINE];

    /* Build the HTTP response body */
    sprintf(body, "<html><title> proxy error</title>");
    sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
    sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
    sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
    sprintf(body, "%s<hr><em>The Proxy Web server</em>\r\n", body);

    /* Print the HTTP response */
    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
    Rio_writen(fd, buf, strlen(buf));
    Rio_writen(fd, body, strlen(body));
}

