//参考　https://www.ibm.com/docs/ja/i/7.2?topic=designs-example-nonblocking-io-select

#include "util.h"

#define BUF_LEN 256             /* バッファのサイズ */
#define TRUE             1
#define FALSE            0

fd_set master_set;
int max_sd;
Descriptor *descriptor_array[5000];

void _closeConnection(int descriptor){
    close(descriptor);
    FD_CLR(descriptor, &master_set);
    if (descriptor == max_sd)
    {
       while (FD_ISSET(max_sd, &master_set) == FALSE)
         max_sd -= 1;
    }
}

void send_http_request(int descriptor, Request* request) {
  int port = 3000;
  

  char request_header[1000];
  snprintf(request_header, 
      1000, "GET %s HTTP/1.1\n"
      "Host: localhost:3000\n"
      "Connection: keep-alive\n"
      "Connection: keep-alive\n"
      "Cache-Control: max-age=0\n"
      "sec-ch-ua: ' Not;A Brand';v='99', 'Google Chrome';v='97', 'Chromium';v='97'\n"
      "sec-ch-ua-mobile: ?0\n"
      "sec-ch-ua-platform: 'macOS'\n"
      "Upgrade-Insecure-Requests: 1\n"
      "User-Agent: Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/97.0.4692.99 Safari/537.36\n"
      "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.9\n"
      "Sec-Fetch-Site: none\n"
      "Sec-Fetch-Mode: navigate\n"
      "Sec-Fetch-User: ?1\n"
      "Sec-Fetch-Dest: document\n"
      "Accept-Encoding: gzip, deflate, br\n"
      "Accept-Language: ja,en-US;q=0.9,en;q=0.8\n"
      "Cookie: _ga=GA1.1.910668344.1636532212; \n"
      "If-None-Match: W/'e6-dDWBSrgCLZdftT+LVe2p582pVks'\n\n"
      ,request->url_path
   );

  char *ip = "127.0.0.1";
  int len = strlen(request_header);
  int sock, on = 1;
  
  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
     perror("socketに失敗");

  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = inet_addr(ip);
  addr.sin_port = htons(port);
  
  if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
     printf("connectに失敗 errno = %d\n", errno);
   

  if (send(sock, request_header, len, 0) == -1)
     perror("sendに失敗");
  ;

  
  int total = 0;
  int num;
  char buf[100000];
  int size = 0;

  while(1){
      memset(buf, 0, sizeof(buf));

      int rc = recv(sock, buf, sizeof(buf), 0);
      printf("RECEIVE: \n");

      if (rc < 0){
         if (errno != EWOULDBLOCK){
            perror("  recv() failed");
            //sclose_conn = TRUE;
         }
         else{
            printf("読み込めるデータがない\n");
         }
         break;
      }
      else if (rc == 0){
         printf("  Connection closed\n");
         close(sock);
         //リクエストを全部送ったことを知らせる。
         _closeConnection(descriptor);
         break;
      }
      //printf("%s\n", buf);
      rc = send(descriptor, buf, strlen(buf), 0);
      
      printf("SEND!!\n");
      if (rc < 0){
         perror("  send() failed");
      }
      size += rc;
  }
  
  }

int main(int argc, char *argv[]){
    int connected_socket, listening_socket;
    struct sockaddr_in sin;
    int len, ret;
    int on = 1;
    int port = 8000;
    int new_sd;
    fd_set working_set;
    struct timeval timeout;
    int    close_conn;
    char   buffer[2048];
    Request* request;

    //socket 作成　返り値はファイルディスクリプタ
    listening_socket = socket(AF_INET, SOCK_STREAM, 0);
    if ( listening_socket == -1 ){
        perror("socket");
        exit(1);
    }
	//ソケットオプションの設定。https://man.plustar.jp/php/function.socket-set-option.html　
    //第４引数はオプションの値.SO_REUSEADDRはboolの整数値（0か1）をとる
    if ( setsockopt(listening_socket, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) == -1 ){
	    perror("socket オプションの設定に失敗しました");
        exit(1);
    }

    /*************************************************************/
    /* Set socket to be nonblocking. All of the sockets for      */
    /* the incoming connections will also be nonblocking since   */
    /* they will inherit that state from the listening socket.   */
    /*************************************************************/
    //ソケットをノンブロッキングモードにする　https://www.geekpage.jp/programming/linux-network/nonblocking.php
    //fcntlでも代用可
    if (ioctl(listening_socket, FIONBIO, (char *)&on) < 0)
    {
       perror("ioctl() failed");
       close(listening_socket);
       exit(-1);
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


    FD_ZERO(&master_set);
    max_sd = listening_socket;
    FD_SET(listening_socket, &master_set);

    /*************************************************************/
    /* Initialize the timeval struct to 3 minutes.  If no        */
    /* activity after 3 minutes this program will end.           */
    /*************************************************************/
    timeout.tv_sec  = 3 * 60;
    timeout.tv_usec = 0;

    while (1){

        /**********************************************************/
      /* Copy the master fd_set over to the working fd_set.     */
      /**********************************************************/
      memcpy(&working_set, &master_set, sizeof(master_set));
      new_sd = 0;
      /**********************************************************/
      /* Call select() and wait 3 minutes for it to complete.   */
      /**********************************************************/
      printf("Waiting on select()...\n");
      int rc = select(max_sd + 1, &working_set, NULL, NULL, &timeout);

      /**********************************************************/
      /* Check to see if the select call failed.                */
      /**********************************************************/
      if (rc < 0)
      {
         perror("  select() failed");
         break;
      }

      /**********************************************************/
      /* Check to see if the 3 minute time out expired.         */
      /**********************************************************/
      if (rc == 0)
      {
         printf("  select() timed out.  End program.\n");
         break;
      }

      /**********************************************************/
      /* One or more descriptors are readable.  Need to         */
      /* determine which ones they are.                         */
      /**********************************************************/
      int desc_ready = rc;
      
      for (int i = 0; i <= max_sd && desc_ready > 0; ++i)
      {
         /*******************************************************/
         /* Check to see if this descriptor is ready            */
         /*******************************************************/
         if (FD_ISSET(i, &working_set))
         {
            /****************************************************/
            /* A descriptor was found that was readable - one   */
            /* less has to be looked for.  This is being done   */
            /* so that we can stop looking at the working set   */
            /* once we have found all of the descriptors that   */
            /* were ready.                                      */
            /****************************************************/
            desc_ready -= 1;

            /****************************************************/
            /* Check to see if this is the listening socket     */
            /****************************************************/
            if (i == listening_socket)
            {
               printf("  Listening socket is readable\n");
               
               /*************************************************/
               /* Accept all incoming connections that are      */
               /* queued up on the listening socket before we   */
               /* loop back and call select again.              */
               /*************************************************/
               while (new_sd != -1)//「グローバル」変数は初期値がない場合は0に初期化される。ローカル変数は何が入るかわからない
               {
                   
                   /**********************************************/
                   /* Accept each incoming connection.  If       */
                   /* accept fails with EWOULDBLOCK, then we     */
                   /* have accepted all of them.  Any other      */
                   /* failure on accept will cause us to end the */
                   /* server.                                    */
                   /**********************************************/
                   
                   new_sd = accept(listening_socket, NULL, NULL);
                   
                   if (new_sd < 0)
                   {
                       if (errno != EWOULDBLOCK)
                       {
                           perror("  accept() failed");
                       }
                       
                       break;
                  }

                  /**********************************************/
                  /* Add the new incoming connection to the     */
                  /* master read set                            */
                  /**********************************************/
                  printf("  New incoming connection - %d\n", new_sd);
                  FD_SET(new_sd, &master_set);
                  if (new_sd > max_sd)
                     max_sd = new_sd;

                  /**********************************************/
                  /* Loop back up and accept another incoming   */
                  /* connection                                 */
                  /**********************************************/
               } 
               
            }
            
            /****************************************************/
            /* This is not the listening socket, therefore an   */
            /* existing connection must be readable             */
            /****************************************************/
            else
            {
               printf("  Descriptor %d is readable\n", i);
               
               /*************************************************/
               /* Receive all incoming data on this socket      */
               /* before we loop back and call select again.    */
               /*************************************************/
               
               do
               {
                  /**********************************************/
                  /* Receive data on this connection until the  */
                  /* recv fails with EWOULDBLOCK.  If any other */
                  /* failure occurs, we will close the          */
                  /* connection.                                */
                  /**********************************************/
                 
                  rc = recv(i, buffer, sizeof(buffer), 0);
                  
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

                  /**********************************************/
                  /* Check to see if the connection has been    */
                  /* closed by the client                       */
                  /**********************************************/
                  if (rc == 0)
                  {
                     printf("  Connection closedされました %i\n", i);

                     _closeConnection(i);
                     break;
                  }

                  /**********************************************/
                  /* Data was received                          */
                  /**********************************************/
                  len = rc;
                  
                  printf("  %d bytes received\n", len);
                  
                  
                  //メモリを解放するのを忘れないこと。
                  //todo リクエストが分割して送られて来た時の対応
                  request = (Request*)calloc(sizeof(Request),1);

                  http_parse(buffer, request);
                  
                  send_http_request(i, request);
                  free(request);
                  //send_http_requestで既にconnection close してる
                  break;
               } while (TRUE);
            } 
         } 
      } 
    }
    ret = close(listening_socket);
    if ( ret == -1 ){
        perror("close");
        exit(1);
    }

    return 0;
}

