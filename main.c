//参考　https://www.ibm.com/docs/ja/i/7.2?topic=designs-example-nonblocking-io-select

#include "util.h"
#include <sys/epoll.h>

#define BUF_LEN 256             /* バッファのサイズ */
#define TRUE             1
#define FALSE            0
#define MAX_EVENTS 10 

struct epoll_event ev, events[MAX_EVENTS]; 
int epollfd;
static struct Descriptor descriptor_array[5000];
static unsigned int ip_addr;
static int port = 3000;
static char *hostname = "app";

void _closeConnection(int descriptor){
   printf("close connection%d\n", descriptor);
   close(descriptor);
   epoll_ctl(epollfd, EPOLL_CTL_DEL, descriptor, NULL);

   descriptor_array[descriptor_array[descriptor].num].status = CLOSE;
}

/**
 * 接続先のipアドレスを解決する
 * 
 */
void _setup(){
    struct addrinfo hints, *res;
    struct in_addr addrs;

    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_family = AF_INET;

    if ((getaddrinfo(hostname, NULL, &hints, &res)) != 0) {
        printf("getaddrinf failed\n");
        exit(1);
    }
    addrs.s_addr= ((struct sockaddr_in *)(res->ai_addr))->sin_addr.s_addr;

    //printf("ip addres resolve = : %s\n", inet_ntoa(addrs));
    ip_addr = addrs.s_addr;
}

void _makeNonBlocking(int descriptor){
   int on = 1;
   //ソケットをノンブロッキングモードにする　https://www.geekpage.jp/programming/linux-network/nonblocking.php
   //fcntlでも代用可
   if (ioctl(descriptor, FIONBIO, (char *)&on) < 0)
   {
      perror("ioctl() failed");
      close(descriptor);
      exit(-1);
   }
}

void send_http_request(int descriptor, char* buffer) {

   if (!descriptor_array[descriptor].status)
   {
      int sock;
      if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) perror("socketに失敗");

      struct sockaddr_in addr;
      addr.sin_family = AF_INET;
      addr.sin_addr.s_addr = ip_addr;
      addr.sin_port = htons(port);

         
      
      if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) printf("connectに失敗 errno = %d\n", errno);
      printf("サーバーとコネクト成功。descriptor = %d\n", sock);
      _makeNonBlocking(sock);
      
      ev.events = EPOLLIN | EPOLLET;
      ev.data.fd = sock;
      if (epoll_ctl(epollfd, EPOLL_CTL_ADD, sock,
                  &ev) == -1) {
          perror("epoll_ctl: conn_sock");
          exit(EXIT_FAILURE);
      }
      
      descriptor_array[sock].num = descriptor;
      descriptor_array[sock].is_from_server = 0;
      descriptor_array[sock].status = OPEN;
      descriptor_array[descriptor].num = sock;
      descriptor_array[descriptor].status = OPEN;
   }
   if (send(descriptor_array[descriptor].num, buffer, strlen(buffer), 0) == -1) perror("sendに失敗");
   printf("サーバーに%ldバイト転送した\n", strlen(buffer));
  
}

void _acceptNewInComingSocket(int listening_socket){
   printf("  Listening socket is readable\n");
   int conn_sock = accept(listening_socket,NULL, NULL);
   if (conn_sock == -1) {
      perror("accept");
      exit(EXIT_FAILURE);
   }      
   
   _makeNonBlocking(conn_sock);
   ev.events = EPOLLIN | EPOLLET;
   ev.data.fd = conn_sock;
   if (epoll_ctl(epollfd, EPOLL_CTL_ADD, conn_sock,&ev) == -1) {
      perror("epoll_ctl: conn_sock");
      exit(EXIT_FAILURE);
   }

   descriptor_array[conn_sock].is_from_server = 1;
   printf("  New incoming connection - %d\n", conn_sock);
}

void _getDataFromServerAndSendDataToClient(int sock){
   char buf[5000];
   while(1){
      memset(buf, 0, sizeof(buf));
      int rc = recv(sock, buf, sizeof(buf), 0);
      printf("RECEIVE %d バイト \n",rc);
      if (rc < 0){
         if (errno != EWOULDBLOCK){
            perror("  recv() failed");
         }
         else{
            printf("読み込めるデータがない\n");
         }
         break;
      }
      else if (rc == 0){
         printf("  Connection closed\n");
         _closeConnection(sock);
         //リクエストを全部送ったことを知らせる。
         _closeConnection(descriptor_array[sock].num);
         break;
      }

      rc = send(descriptor_array[sock].num, buf, strlen(buf), 0); 
      if (rc < 0){
         perror("  send() failed");
      }
      printf("クライアントにサーバからのデータを転送した!\n");  
   }
}

void _getDataFromClientAndSendDataToServer(int sock){
   printf("  Descriptor %d is readable\n", sock);           
   while(1)
   {
      char buffer[5000];
      memset(buffer, 0, sizeof(buffer));
      int rc = recv(sock, buffer, sizeof(buffer), 0);
      
      if (rc < 0)
      {
         if (errno != EWOULDBLOCK)
         {
            perror("  recv() failed");
         }
         else{
             printf("読み込めるデータがありません\n");
             //_closeConnection(i);
         }
         break;
      }

      if (rc == 0)
      {
         printf("  Connection closedされました %d\n", sock);
         _closeConnection(sock);
         break;
      }
      
      printf("  %d bytes received\n", rc);
      
      send_http_request(sock, buffer);
   } 
}



int main(int argc, char *argv[]){
   _setup();

   int listening_socket;
   struct sockaddr_in sin;
   int ret;
   int on = 1;
   int port = 80;
   //Request *request;
   int nfds;

   // socket 作成　返り値はファイルディスクリプタ
   listening_socket = socket(AF_INET, SOCK_STREAM, 0);
   if (listening_socket == -1)
   {
      perror("socket");
      exit(1);
    }
	//ソケットオプションの設定。https://man.plustar.jp/php/function.socket-set-option.html　
    //第４引数はオプションの値.SO_REUSEADDRはboolの整数値（0か1）をとる
    if ( setsockopt(listening_socket, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) == -1 ){
	    perror("socket オプションの設定に失敗しました");
        exit(1);
    }

    //スタック上のローカル変数の初期値は不定だから初期化https://neineigh.hatenablog.com/entry/2013/09/28/185053
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(port);
    sin.sin_addr.s_addr = htonl(INADDR_ANY);//どのipアドレスからのパケットも受け付ける　https://www.wdic.org/w/WDIC/INADDR_ANY

	/* ソケットにアドレスを割り当てる */
    if ( bind(listening_socket, (struct sockaddr *)&sin, sizeof(sin)) < 0 ){
	    perror("bind");
        exit(1);
    }
	//デフォルトだとmax connection が128に設定されるので注意！！！！！https://christina04.hatenablog.com/entry/2016/12/31/124314
    ret = listen(listening_socket, SOMAXCONN);
    if ( ret == -1 ){
	perror("listenが失敗");
        exit(1);
    }
    printf("ポート番号%dでlisten start !!!。\n", port);

    epollfd = epoll_create1(0);
    if (epollfd == -1)
    {
        perror("epoll_create1");
        exit(EXIT_FAILURE);
    }

    ev.events = EPOLLIN; ev.data.fd = listening_socket; if (epoll_ctl(epollfd, EPOLL_CTL_ADD, listening_socket, &ev) == -1) {
        perror("epoll_ctl: listen_sock");
        exit(EXIT_FAILURE); }

    while (1){
      printf("Waiting on epoll()...\n");

      nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);
      if (nfds == -1) {
          perror("epoll_wait");
          exit(EXIT_FAILURE);
      }
      
      for (int n = 0; n < nfds; ++n) {
         int sock = events[n].data.fd;
         if (sock == listening_socket) _acceptNewInComingSocket(listening_socket);

         //サーバからデータが送られてきた場合
         else if(!descriptor_array[sock].is_from_server) _getDataFromServerAndSendDataToClient(sock);
         else _getDataFromClientAndSendDataToServer(sock);     
      } 
   }
}

