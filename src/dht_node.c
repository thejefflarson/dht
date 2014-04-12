#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "dht_node.h"

dht_node_t *
dht_node_new(char id[32]) {
  dht_node_t* node = (dht_node_t*) malloc(sizeof(dht_node_t));

  if(node == NULL)
    return NULL;

  strncpy(node->id, id, 32);
  time(&node->created_at);
  time(&node->last_heard);
  return node;
}

void
dht_node_free(dht_node_t* node){
  free(node);
}
