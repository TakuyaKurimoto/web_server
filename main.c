//epoll はこれを参考にした　https://raskr.hatenablog.com/entry/2018/04/21/143825
#include "util.h"


#define BUF_LEN 256             /* バッファのサイズ */
#define TRUE             1
#define FALSE            0
#define MAX_EVENTS 20 
#define CLIENT 0
#define SERVER 1


struct epoll_event ev, events[MAX_EVENTS]; 
int epollfd;
struct Request *descriptor_array[5000];
static unsigned int ip_addr;
json_t *config;
json_error_t jerror;
static int port;
static char* hostname;
//ローカル変数にしてしまうと、毎回メモリ確保で遅くなるので、グローバル変数にしてしまう。
static char buf[4096];
int buffer_len, buf_len;
const char *method, *path;
int pret, minor_version;
struct phr_header headers[100];
size_t buflen, prevbuflen, method_len, path_len, num_headers;
ssize_t rc;
int CACHE, CACHE_EXPIRE;

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

void send_http_request(int descriptor) {
   struct Request *req = descriptor_array[descriptor];
   if (!req->status)
   {
      int sock;
      if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) perror("socketに失敗");
      struct sockaddr_in addr;
      addr.sin_family = AF_INET;
      addr.sin_addr.s_addr = ip_addr;
      addr.sin_port = htons(port);
      
      if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) printf("connectに失敗 errno = %d\n", errno);
      printf("サーバーとコネクト成功。descriptor = %d\n", sock);
      makeNonBlocking(sock);
      
      ev.events = EPOLLIN | EPOLLET;
      ev.data.fd = sock;
      if (epoll_ctl(epollfd, EPOLL_CTL_ADD, sock, &ev) == -1) {
         perror("epoll_ctl: conn_sock");
         exit(EXIT_FAILURE);
      }
      descriptor_array[sock]->num = descriptor;
      descriptor_array[sock]->is_from_server = 0;
      descriptor_array[sock]->status = OPEN;
      req->num = sock;
      req->status = OPEN;
   }
   if (send(req->num, req->buffer, req->request_size, 0) == -1) printf("sendに失敗。sock=%d\n", req->num);
   // printf("%.*s\n", req->request_size, req->buffer);
   printf("サーバーに%dバイト転送した\n", req->buflen);
}

void _acceptNewInComingSocket(int listening_socket){
   printf("  Listening socket is readable\n");
   int conn_sock = accept(listening_socket,NULL, NULL);
   if (conn_sock == -1) {
      perror("accept");
      exit(EXIT_FAILURE);
   }      
   
   makeNonBlocking(conn_sock);
   ev.events = EPOLLIN | EPOLLET;
   ev.data.fd = conn_sock;
   if (epoll_ctl(epollfd, EPOLL_CTL_ADD, conn_sock,&ev) == -1) {
      perror("epoll_ctl: conn_sock");
      exit(EXIT_FAILURE);
   }

   descriptor_array[conn_sock]->is_from_server = 1;
   printf("  New incoming connection - %d\n", conn_sock);
}

void _getDataFromServerAndSendDataToClient(int sock){
   int err, rc;
   while(1){
      rc = recv(sock, buf, sizeof(buf), 0);
      buf_len = rc;
      if (rc < 0)
      {
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
         closeConnection(sock);
         //リクエストを全部送ったことを知らせる。
         closeConnection(descriptor_array[sock]->num);
         break;
      }
      else {
         printf("RECEIVE %d バイト \n",rc);
         printf("%s\n", buf);
         err = send(descriptor_array[sock]->num, buf, buf_len, 0);
         if (err < 0){
            perror("  send() failed");
         }
         printf("クライアントにサーバからのデータを転送した!\n");
         if (descriptor_array[descriptor_array[sock]->num]->buflen > 0) initReq(descriptor_array[sock]->num);
      }
   }
}

void _getDataFromClientAndSendDataToServer(int sock){
   struct Request *req = descriptor_array[sock];
   while (1)
   {
      rc = read(sock, req->buffer + req->buflen, req->buffer_size - req->buflen);
      printf("%ldbyte receive\n", rc);
      req->prevbuflen = req->buflen;
      if(rc >= 0){
         req->buflen += rc;
      }
      /* parse the request */
      num_headers = sizeof(headers) / sizeof(headers[0]);
      pret = phr_parse_request(req->buffer, req->buflen, &method, &method_len, &path, &path_len,
                               &minor_version, headers, &num_headers, req->prevbuflen);

      printf("pret = %d\n", pret);
      if (rc < 0)
      {
         if (errno != EWOULDBLOCK)
         {
            perror("  read() failed");
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
         // https://www.ibm.com/docs/ja/zos/2.3.0?topic=functions-read-read-from-file-socket#:~:text=%E5%88%B0%E7%9D%80%E3%81%99%E3%82%8B%E3%81%BE%E3%81%A7%E3%80%81-,read,-()%20%E5%91%BC%E3%81%B3%E5%87%BA%E3%81%97%E3%81%AF%E5%91%BC%E3%81%B3%E5%87%BA%E3%81%97
         printf("  Connection closedされました %d\n", sock);
         closeConnection(sock);
         break;
      }     
      if (req->request_size == 0 && pret > 0) {
         for (size_t i = 0; i != num_headers; ++i) {
            if (!strncmp(headers[i].name, "Content-Length", (int)headers[i].name_len) || !strncmp(headers[i].name, "Content-length", (int)headers[i].name_len))
            {
               if (pret + atoi(headers[i].value) + 1 > 4096) {
                  char *tmp;
                  // 1byte余分に確保しないとread()の返り値が本来-1になる場合も0になってしまう
                  if ((tmp = (char *)realloc(req->buffer, pret + atoi(headers[i].value) + 1)) == NULL) {
                     printf("realloc時にメモリが確保できません\n");
                     break;
                  }
                  printf("buffer_sizeを%dに拡大\n", pret + atoi(headers[i].value) + 1);
                  /* reallocの戻り値は一度別変数に取り、
                     NULLでないことを確認してから元の変数に代入するのが定石 */
                  req->buffer = tmp;
                  req->buffer_size = pret + atoi(headers[i].value) + 1;
               }
               req->request_size = pret + atoi(headers[i].value);
            }
         }
         
         if (req->request_size == 0) req->request_size = pret;
      }
      // requestを全て受け切った
      if (req->request_size  == req->buflen) {
         send_http_request(sock);
         break;
      }
   }
   /*
   if (req->request_size  == req->buflen) {
      printf("request is %d bytes long\n", pret);
   
      printf("method is %.*s\n", (int)method_len, method);
      printf("path is %.*s\n", (int)path_len, path);
      printf("HTTP version is 1.%d\n", minor_version);
      printf("headers:\n");
      for (size_t i = 0; i != num_headers; ++i) {
         printf("%.*s: %.*s\n", (int)headers[i].name_len, headers[i].name,
               (int)headers[i].value_len, headers[i].value);
      }
   }
   */
}




int main(int argc, char *argv[]){
   // port設定
   config = json_load_file("config.json",0,&jerror);
   int self_port = json_integer_value(json_object_get(config, "self_port"));
   port = json_integer_value(json_object_get(config, "forwarding_port"));
   hostname = (char*)json_string_value(json_object_get(config, "forwarding_hostname"));

   CACHE = json_integer_value(json_object_get(config, "cache"));
   if (CACHE) {
      CACHE_EXPIRE = json_integer_value(json_object_get(config, "cache_expire"));
      printf("cache_expire=%d\n", CACHE_EXPIRE);
   } 
   
   _setup();

   for (int i = 0; i < 5000; i++){
      struct Request *req = descriptor_array[i] = calloc(1, sizeof(struct Request));
      
      char *str = (char *)calloc(4096, sizeof(char));
      if(str == NULL) {
         printf("メモリが確保できません\n");
      }
      req->buffer = str;
      req->buflen = 0;
      req->prevbuflen = 0;
      req->buffer_size = 4096;
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
      if (nfds == -1)
      {
         perror("epoll_wait");
         exit(EXIT_FAILURE);
      }

      for (int n = 0; n < nfds; ++n) {
         int sock = events[n].data.fd;
         if (sock == listening_socket) { 
            _acceptNewInComingSocket(listening_socket);
            continue;
         }
         printf("Request %d is readable\n", sock);
         //サーバからデータが送られてきた場合
         if(!descriptor_array[sock]->is_from_server) _getDataFromServerAndSendDataToClient(sock);
         else _getDataFromClientAndSendDataToServer(sock);
      } 
   }
}

