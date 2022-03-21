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

#define OPEN          1
#define CLOSE         0

void http_parse();

typedef struct  {          
   char url_path[30];        
} Request;

struct Descriptor {          
   int is_from_server;
   int num;
   int status;
};