#include <memory.h>
#include <time.h>
#include "dht_node.h"

dht_node_t *
dht_node_new() {
  dht_node_t* node = (dht_node_t*) malloc(sizeof(dht_node_t));
  if(node == NULL)
    return null;

  time(&node->created_at);
  time(&node->last_heard);
  return node;
}
