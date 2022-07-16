#include "util.h"
extern int epollfd;
extern struct Descriptor descriptor_array[5000];

void closeConnection(int descriptor)
{
    printf("close connection%d\n", descriptor);
    close(descriptor);
    epoll_ctl(epollfd, EPOLL_CTL_DEL, descriptor, NULL);
    descriptor_array[descriptor].status = CLOSE;
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