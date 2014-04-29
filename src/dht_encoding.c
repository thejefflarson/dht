#include <memory.h>

dht_be_node *
dht_be_decode(char *str, long long size){
  int err = 0;
  dht_be_node *node = calloc(1, sizeof(dht_be_node));
  while(size-- && err == 0) {
    switch(*str){
      case '':

        break;
      case '':

        break;
      default:
        err = 1;
        break;
    }

    str++;
  }

  if(err) {
    dht_be_free(node);
    return NULL;
  } else {
    return node;
  }
}

char *
dht_be_encode(dht_be_node *node){

}

void
dht_be_free(dht_be_node *node) {

}