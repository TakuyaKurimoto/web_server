/*
#include "util.h"

char* get_url_path(char* str, char *url_path){
    str += strlen("GET ");
    int url_length = 0;
    while (strncmp(" ", str+url_length, 1)){
        url_length += 1;
    }
    strncpy(url_path, str, url_length);

    str += url_length;
    while (strncmp("\n", str, 1))
    {
        str += 1;
    }
    return str;
}

void http_parse(char* str, Request* request){
    
    
    if(!strncmp("GET", str, 3)){
        str = get_url_path(str, request->url_path);
        
    }

    
    while (*str)
    {
        if(strncmp("\r\n\r\n", str, 4)){
            str += 1;
        }
        else
        {
            printf("read all header\n");
            break;
        }
    }
    //printf("url_path = %s\n", request->url_path);
    return;
}
 // HTTP versionを確認
 ptr = buf + 5;
 if (strncmp("1.0 ", ptr, 4) == 0) {
	 printf("is HTTP/1.0\n");
	 ptr += 4;
 */
