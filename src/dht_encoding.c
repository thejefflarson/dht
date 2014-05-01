#include <memory.h>

dht_be_node *
dht_be_decode(char *str, long long size){
  int err = 0;
  dht_be_node *node = calloc(1, sizeof(dht_be_node));
  while(size > 0 && err == 0) {
    switch(*str){
      case 'i':
        node->val.i = parse_int(&str, &size, &err);
        if(*str != 'e') err = 1;
        break;
      case 'l':
        decode_list(node, &str, &size, &err);
        break;
      case 'd':
        decode_dictionary(node, &str, &size, &err);
        break;
      default:
        if(peek(str, size, ':')) {
          node->val.str = decode_string(&str, &size, &err);
        } else {
          err = 1;
        }
        break;
    }
    if(*str != 'e') {
      err = 1;
    } else {
      str++;
    }
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
  free(node);
}