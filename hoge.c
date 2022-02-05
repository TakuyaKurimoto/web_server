#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void main(){
char *huga = "hogehoge\nhoge";

  printf("%s",strtok(huga, '\n'));
}