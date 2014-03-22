#include <memory.h>
#include <time.h>
#include "dht_node.h"
#include "dht_util.h"

dht_node_t *
dht_node_new(char[32] id) {
  dht_node_t* node = (dht_node_t*) malloc(sizeof(dht_node_t));

  if(node == NULL)
    return null;

  node->id = id;
  time(&node->created_at);
  time(&node->last_heard);
  return node;
}
