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

#define BUF_LEN 256             /* バッファのサイズ */
#define TRUE             1
#define FALSE            0


int main() {
  int port = 3000;
  char *mes = "GET /user HTTP/1.1\n"
              "Host: localhost:3000\n"
              "Connection: keep-alive\n"
              "\n";
  char *ip = "127.0.0.1";
  int len = strlen(mes);
  int sock, on = 1;

  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
     perror("socketに失敗");

  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = inet_addr(ip);
  addr.sin_port = htons(port);
  
  if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
     printf("connectに失敗 errno = %d\n", errno);
   

  if (send(sock, mes, len, 0) == -1)
     perror("sendに失敗");
  ;

  printf("RECEIVE: \n");
  int total = 0;
  int num;
  char buf[100000];

  int rc = recv(sock, buf, sizeof(buf), 0);
  printf("REIVE: \n");
  if (rc < 0)
  {
     if (errno != EWOULDBLOCK)
     {
        perror("  recv() failed");
        //sclose_conn = TRUE;
     }
     else
     {
        printf("リクエストを全部読み込んだ\n");
     }
   }
   printf("hogeho\n", errno);
   printf("%s\n", buf);
   printf("\n");
   close(sock);
   exit(1);
   return 1;
}


