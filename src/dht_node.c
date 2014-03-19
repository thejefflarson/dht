#include <memory.h>
#include "dht_node.h"

dht_node_t *
dht_node_new() {
  dht_node_t* node = (dht_node_t*) malloc(sizeof(dht_node_t));
  return node;
}
