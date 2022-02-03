/*
 * $Id: echo-server-1.c,v 1.6 2005/02/19 16:01:53 68user Exp $
 *
 * echo サ〖バサンプル
 *
 * written by 68user  http://X68000.q-e-d.net/~68user/
 */
//参考　https://www.ibm.com/docs/ja/i/7.2?topic=designs-example-nonblocking-io-select

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

void _closeConnection(int descriptor, fd_set* master_set, int* max_sd){
    close(descriptor);
    FD_CLR(descriptor, master_set);
    if (descriptor == *max_sd)
    {
       while (FD_ISSET(*max_sd, master_set) == FALSE)
          *max_sd -= 1;
    }
}


int main(int argc, char *argv[]){
    int connected_socket, listening_socket;
    struct sockaddr_in sin;
    int len, ret;
    int on = 1;
    int port = 8000;
    int max_sd, new_sd;
    fd_set master_set, working_set;
    struct timeval timeout;
    int    close_conn;
    char   buffer[2048];


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
               
               close_conn = FALSE;
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
                        //sclose_conn = TRUE;
                     }
                     else{
                         printf("リクエストを全部読み込んだ\n");
                     }
                     _closeConnection(i, &master_set, &max_sd);


                  break;
                  }

                  /**********************************************/
                  /* Check to see if the connection has been    */
                  /* closed by the client                       */
                  /**********************************************/
                  if (rc == 0)
                  {
                     printf("  Connection closed %i\n", i);
                     //close_conn = TRUE;

                     _closeConnection(i, &master_set, &max_sd);
                     break;
                  }

                  /**********************************************/
                  /* Data was received                          */
                  /**********************************************/
                  len = rc;
                  printf("  %d bytes received\n", len);
                  
                  /**********************************************/
                  /* Echo the data back to the client           */
                  /**********************************************/
                  char *reply = 
                    "HTTP/1.1 200 OK\n"
                    "Date: Thu, 19 Feb 2009 12:27:04 GMT\n"
                    "Server: Apache/2.2.3\n"
                    "Last-Modified: Wed, 18 Jun 2003 16:05:58 GMT\n"
                    "ETag: \"56d-9989200-1132c580\"\n"
                    "Content-Type: text/html\n"
                    "Content-Length: 15\n"
                    "Accept-Ranges: bytes\n"
                    "Connection: close\n"
                    "\n"
                    "sdfkjsdnbfkjbsf";
                  rc = send(i, reply, strlen(reply), 0);
                  

                  
                  if (rc < 0)
                  {
                     perror("  send() failed");
                     close_conn = TRUE;
                     break;
                  }
                  
               } while (TRUE);

               /*************************************************/
               /* If the close_conn flag was turned on, we need */
               /* to clean up this active connection.  This     */
               /* clean up process includes removing the        */
               /* descriptor from the master set and            */
               /* determining the new maximum descriptor value  */
               /* based on the bits that are still turned on in */
               /* the master set.                               */
               /*************************************************/
               
               if (close_conn)
               {
                  close(i);
                  FD_CLR(i, &master_set);
                  if (i == max_sd)
                  {
                     while (FD_ISSET(max_sd, &master_set) == FALSE)
                        max_sd -= 1;
                  }
                  
                }
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

