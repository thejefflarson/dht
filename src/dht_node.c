#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "dht_node.h"

dht_node_t *
dht_node_new(unsigned char id[32]) {
  dht_node_t* node = calloc(1, sizeof(dht_node_t));

  if(node == NULL)
    return NULL;

  strncpy((char *)node->id, (char *)id, 32);
  time(&node->created_at);
  time(&node->last_heard);
  return node;
}

void
dht_node_free(dht_node_t *node){
  free(node);
}

bool
dht_node_good(dht_node_t *node){
  return time(NULL) - node->last_heard < 1500;
}


int
dht_node_compare(dht_node_t *a, dht_node_t *b){
  for(int i = 0; i < 32; i++){
    uint8_t aint = a->id[i], bint = b->id[i];

    if(aint == bint) continue;

    return aint > bint ? 1 : -1;
  }
  return 0
}