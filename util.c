#include "util.h"
extern int epollfd;
extern struct Request *descriptor_array[5000];

void closeConnection(int descriptor)
{
    printf("close connection%d\n", descriptor);
    close(descriptor);
    epoll_ctl(epollfd, EPOLL_CTL_DEL, descriptor, NULL);
    descriptor_array[descriptor]->status = CLOSE;
    initReq(descriptor);
}

void makeNonBlocking(int descriptor){
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

void initReq(int descriptor){
    printf("descriptor=%dを初期化\n", descriptor);
    descriptor_array[descriptor]->buflen = 0;
    descriptor_array[descriptor]->prevbuflen = 0;
    descriptor_array[descriptor]->request_size = 0;
    if (descriptor_array[descriptor]->buffer_size == 4096) {
       memset(descriptor_array[descriptor]->buffer, 0, 4096);
       return;
    }
    free(descriptor_array[descriptor]->buffer);
    char *str = (char *)calloc(4096, sizeof(char));
    if(str == NULL) {
       printf("メモリが確保できません\n");
    }
    descriptor_array[descriptor]->buffer = str;
    descriptor_array[descriptor]->buffer_size = 4096;
}
