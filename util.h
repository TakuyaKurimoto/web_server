#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <unistd.h>
#include <sys/param.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/epoll.h>
#include <jansson.h>
#include <assert.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <openssl/md5.h>
#include "picohttpparser/picohttpparser.h"
#include "include/uthash.h"

#define OPEN          1
#define CLOSE         0

struct Request{          
   int is_from_server;
   // descriptor番号
   int num;
   // socketが生きているか
   int status;
   // http rquestが入る
   char *buffer;
   int buffer_size;
   int buflen;
   int prevbuflen;
   int request_size;
   const char *path;
   size_t path_len;
};

struct Cache{          
   int expire;
   char name[50];
};

// svoid http_parse();
void closeConnection(int descriptor);
void makeNonBlocking(int descriptor);
void initReq(int descriptor);
char *covertMd5Hash(char *src, char *dst);

/*
typedef struct  {
   char url_path[30];
} Request;
*/
