#include <stdlib.h>
#include <string.h>
#include "../include/dht.h"
#include "dht_node.h"
#include "dht_bucket.h"
#include "dht_util.h"

dht_t *
dht_new(){
  dht_t *dht = calloc(1, sizeof(dht_t));
  if(dht == NULL) return NULL;

  dht->bucket = dht_bucket_new(0, 255);
  if(dht->bucket == NULL) {
    free(dht);
    return NULL;
  }

  return dht;
}

void
dht_free(dht_t *dht) {
  free(dht);
}

int
dht_get(dht_t *dht, uint8_t *key, dht_get_callback cb, void *closure) {
  return 0;
}

int
dht_run(dht_t *dht, uint8_t *data, size_t length, struct sockaddr from, size_t from_length) {
  switch(op){
    case DHT_PING:
      dht_node_t* node = dht_find_node(dht, node_id);
      if(node == NULL) return -1;
      dht_node_update(node);
      dht->send(dht, DHT_PONG, NULL, 0, node);
      return 1;
    case DHT_FIND_NODES:
      // todo: implement nearest_nodes and send
      dht_node_list_t* nodes = dht_find_nearest_nodes(dht, node_id, 10);
      if(nodes == NULL) return -1;
      dht->send(dht, DHT_NODES, node, sizeof(node), node);
      return nodes->length;
    case DHT_ANNOUNCE:
      // add to dht
      return 1;
    default:
      return -1;
  }
}

struct _find_state {
  uint8_t target[32];
  dht_node_t *current;
};

static int
_find_walker(void *ctx, dht_bucket_t *root){
  struct _find_state *state = ctx;
  uint8_t adelta[32], bdelta[32];

  for(int i = 0; i < root->length; i++){
    dht_xor(adelta, state->target, root->nodes[i]->id);
    dht_xor(bdelta, state->target, state->current);

    if(dht_compare(adelta, bdelta) == -1)
      state->current = root->nodes[i];
  }

  return 0;
}

dht_node_t *
dht_find_node(dht_t *dht, uint8_t key[32]) {
  if(dht->bucket->length == 0) return NULL;

  struct _find_state state;

  state.current = dht->bucket->nodes[0];
  memcpy(state.target, key, 32);

  dht_bucket_walk((void *) &state, dht->bucket, _find_walker);

  return state.current;
}
