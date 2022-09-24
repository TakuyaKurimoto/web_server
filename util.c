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

// 参考　https://a4dosanddos.hatenablog.com/entry/2015/03/01/191730
char *covertMd5Hash(char *src, char *dst)
{
    MD5_CTX c;
    unsigned char md[MD5_DIGEST_LENGTH];
    int r, i;
    
    r = MD5_Init(&c);
    if(r != 1) {
        perror("init");
        exit(1);
    }
    
    r = MD5_Update(&c, src, strlen(src));
    if(r != 1) {
        perror("update");
        exit(1);
    }
    
    r = MD5_Final(md, &c);
    if(r != 1) {
        perror("final");
        exit(1);
    }
 
    for(i = 0; i < 16; i++)
         sprintf(&dst[i * 2], "%02x", (unsigned int)md[i]);
 
    return dst;
}

