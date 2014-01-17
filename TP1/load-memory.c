#include <stdlib.h>
#include <stdio.h>

int main() {
  int *p;
  while(1) {
    int inc=1024*1024*sizeof(char);
    p=(int*) calloc(1,inc);
    if(!p) break;
  }
}
