#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "dht_node.h"
#include "dht_util.h"

dht_node_t *
dht_node_new(const unsigned char id[32], const struct sockaddr_storage *address) {
  dht_node_t* node = calloc(1, sizeof(dht_node_t));

  if(node == NULL)
    return NULL;

  memcpy((void *) &node->address, (const void *) address, sizeof(struct sockaddr_storage));
  strncpy((char *) node->id, (char *) id, 32);
  time(&node->created_at);
  dht_node_update(node);
  return node;
}

void
dht_node_free(dht_node_t *node){
  free(node);
}

void
dht_node_update(dht_node_t *node){
  time(&node->last_heard);
}

bool
dht_node_good(dht_node_t *node){
  return time(NULL) - node->last_heard < 1500;
}

