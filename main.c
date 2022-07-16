//epoll はこれを参考にした　https://raskr.hatenablog.com/entry/2018/04/21/143825
#include "util.h"
#include <sys/epoll.h>
#include "pthread.h"
#include <jansson.h>
#include <assert.h>
#include "picohttpparser/picohttpparser.h"
#include "include/uthash.h"

#define BUF_LEN 256             /* バッファのサイズ */
#define TRUE             1
#define FALSE            0
#define MAX_EVENTS 20 
#define CLIENT 0
#define SERVER 1


struct epoll_event ev, events[MAX_EVENTS]; 
int epollfd;
int client_epollfd, server_epollfd;
static struct Descriptor descriptor_array[5000];
static unsigned int ip_addr;
json_t *config;
json_error_t jerror;
static int port;
static char* hostname;
//ローカル変数にしてしまうと、毎回メモリ確保で遅くなるので、グローバル変数にしてしまう。
static char buf[5000];
static char buffer[5000];
const char *method, *path;
int pret, minor_version;
struct phr_header headers[100];
size_t buflen, prevbuflen, method_len, path_len, num_headers;
ssize_t rc;
struct my_struct {
  int key;
  int value;

  UT_hash_handle hh;
};
struct my_struct *table = NULL;
struct my_struct *item;

void _closeConnection(int descriptor, int type){
   printf("close connection%d\n", descriptor);
   close(descriptor);
   
   switch (type){
      case CLIENT:
         epoll_ctl(client_epollfd, EPOLL_CTL_DEL, descriptor, NULL);
         break;
      case SERVER:
         epoll_ctl(server_epollfd, EPOLL_CTL_DEL, descriptor, NULL);
         HASH_FIND_INT(table, &(descriptor_array[descriptor_array[descriptor].num].num), item);
         HASH_DEL(table, item);
         printf("freeソケットリストからsock=%dを削除\n", descriptor);
         free(item);
         break;
      default:
         epoll_ctl(epollfd, EPOLL_CTL_DEL, descriptor, NULL);
   }

   //mutex必要かも
   descriptor_array[descriptor].status = CLOSE;
   // descriptor_array[descriptor_array[descriptor].num].status = CLOSE;
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

void send_http_request(int descriptor) {
   if (!descriptor_array[descriptor].status)
   {
      int sock = -1; 
      for (item = table; item != NULL; item = item->hh.next) {
         sock = item->key;
         HASH_DEL(table, item);
         printf("freeソケットリストからsock=%dを削除\n", sock);
         free(item);
         printf("まだ生きてるソケット再利用=%d\n", sock);
         break;
      }
      
      if (sock == -1){
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
         if (epoll_ctl(server_epollfd, EPOLL_CTL_ADD, sock, &ev) == -1) {
            perror("epoll_ctl: conn_sock");
            exit(EXIT_FAILURE);
         }
      }
      descriptor_array[sock].num = descriptor;
      descriptor_array[sock].is_from_server = 0;
      descriptor_array[sock].status = OPEN;
      descriptor_array[descriptor].num = sock;
      descriptor_array[descriptor].status = OPEN;
      }
   if (send(descriptor_array[descriptor].num, buffer, strlen(buffer), 0) == -1) printf("sendに失敗。sock=%d\n", descriptor_array[descriptor].num);
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
   if (epoll_ctl(client_epollfd, EPOLL_CTL_ADD, conn_sock,&ev) == -1) {
      perror("epoll_ctl: conn_sock");
      exit(EXIT_FAILURE);
   }

   descriptor_array[conn_sock].is_from_server = 1;
   printf("  New incoming connection - %d\n", conn_sock);
}

void * getDataFromServerAndSendDataToClient(void *arg){
   struct epoll_event events[MAX_EVENTS]; 
   int nfds, err, rc;

   server_epollfd = epoll_create1(0);

   if (server_epollfd == -1)
   {
      perror("epoll_create1");
      exit(EXIT_FAILURE);
   }

   while (1){
      printf("Waiting on epoll() [server]...\n");

      nfds = epoll_wait(server_epollfd, events, MAX_EVENTS, -1);
      if (nfds == -1) {
          perror("epoll_wait");
          exit(EXIT_FAILURE);
      }

      for (int n = 0; n < nfds; ++n) {
         int sock = events[n].data.fd;

         while(1){
            memset(buf, 0, sizeof(buf));
            rc = recv(sock, buf, sizeof(buf), 0);

            if (rc < 0){
               if (errno != EWOULDBLOCK){
                  perror("  recv() failed");
               }
               else{
                  // 要素の追加
                  item = malloc(sizeof(struct my_struct));
                  item->key = sock;
                  item->value = 1;
                  HASH_ADD_INT(table, key, item);
                  printf("freeソケットリストにsock=%dを追加\n", sock);
                  descriptor_array[descriptor_array[sock].num].status = CLOSE;
                  printf("読み込めるデータがない\n");
               }
               break;
            }
            else if (rc == 0){
               printf("  Connection closed\n");
               _closeConnection(sock, SERVER);
               //リクエストを全部送ったことを知らせる。
               _closeConnection(descriptor_array[sock].num, CLIENT);
               break;
            }

            printf("RECEIVE %d バイト \n",rc);
            err = send(descriptor_array[sock].num, buf, strlen(buf), 0);
            if (err < 0){
               perror("  send() failed");
            }
            printf("クライアントにサーバからのデータを転送した!\n");  
         }
      }
   }
}

void * getDataFromClientAndSendDataToServer(void *arg){
   struct epoll_event events[MAX_EVENTS]; 
   int nfds;
   client_epollfd = epoll_create1(0);

   if (client_epollfd == -1)
   {
      perror("epoll_create1");
      exit(EXIT_FAILURE);
    }
    
   while (1){
      printf("Waiting on epoll() [client]...\n");

      nfds = epoll_wait(client_epollfd, events, MAX_EVENTS, -1);
      if (nfds == -1) {
          perror("epoll_wait");
          exit(EXIT_FAILURE);
      }
      
      for (int n = 0; n < nfds; ++n) {
         int sock = events[n].data.fd;       

         printf("  Descriptor %d is readable\n", sock);

         buflen = 0;
         prevbuflen = 0;
         while (1)
         {
            memset(buffer, 0, sizeof(buffer));
            rc = read(sock, buffer + buflen, sizeof(buffer) - buflen);

            prevbuflen = buflen;
            if(rc >= 0){
               buflen += rc;
            }
            /* parse the request */
            num_headers = sizeof(headers) / sizeof(headers[0]);
            pret = phr_parse_request(buffer, buflen, &method, &method_len, &path, &path_len,
                                    &minor_version, headers, &num_headers, prevbuflen);

            if (rc < 0)
            {
               if (errno != EWOULDBLOCK)
               {
                  perror("  recv() failed");
               }
               else{
                   printf("読み込めるデータがありません\n");
               }
               break;
            }

            if (pret == -1) {
               printf("ParseError\n");
               break;
            }
            
            if (rc == 0)
            {
               printf("  Connection closedされました %d\n", sock);
               _closeConnection(sock, CLIENT);
               break;
            }     
            printf("  %ld bytes received\n", rc);

            send_http_request(sock);
            if (pret > 0)
               break; /* successfully parsed the request */
         }
         /*
         if(rc != 0) {
            printf("request is %d bytes long\n", pret);
            printf("method is %.*s\n", (int)method_len, method);
            printf("path is %.*s\n", (int)path_len, path);
            printf("HTTP version is 1.%d\n", minor_version);
            printf("headers:\n");
            for (size_t i = 0; i != num_headers; ++i) {
               printf("%.*s: %.*s\n", (int)headers[i].name_len, headers[i].name,
                     (int)headers[i].value_len, headers[i].value);
            }
         }*/
      }
   } 
}



int main(int argc, char *argv[]){
   int err;

   // port設定
   config = json_load_file("config.json",0,&jerror);
   int self_port = json_integer_value(json_object_get(config, "self_port"));
   port = json_integer_value(json_object_get(config, "forwarding_port"));
   hostname = (char*)json_string_value(json_object_get(config, "forwarding_hostname"));
   
   _setup();
   
   pthread_t thread1, thread2;

   err = pthread_create(&thread1, NULL, getDataFromClientAndSendDataToServer, NULL);
   if (err) {
        printf("pthread_create() failure, err=%d", err);
        return -1;
    }
   
   err = pthread_create(&thread2, NULL, getDataFromServerAndSendDataToClient, NULL);
   if (err) {
        printf("pthread_create() failure, err=%d", err);
        return -1;
    }

   int listening_socket;
   struct sockaddr_in sin;
   int ret;
   int on = 1;

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
    sin.sin_port = htons(self_port);
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
    printf("ポート番号%dでlisten start !!!。\n", self_port);

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
      printf("Waiting on epoll() [listening socket]\n");

      nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);
      if (nfds == -1) {
          perror("epoll_wait");
          exit(EXIT_FAILURE);
      }
      
      for (int n = 0; n < nfds; ++n) {
         _acceptNewInComingSocket(listening_socket);  
      } 
   }
}

