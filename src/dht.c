#include <stdlib.h>
#include "dht.h"
#include "dht_node.h"
#include "dht_bucket.h"

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

int
dht_get(dht_t *dht, char *key, dht_get_callback cb) {
  return 0;
}

void
dht_free(dht_t *dht) {
  free(dht)
}

static int
_ping(dht_t *dht, node->id) {
}

int
dht_recv(dht_t *dht, dht_op_t op, char *data, int length, unsigned char node_id[32]) {
  switch(op){
    case DHT_PING:
      dht_node_t* node = dht_find_node(dht, node_id);
      if(node == NULL) return 1;
      dht_node_update(node);
      return 0;
    default:
      break;
  }
}


static struct _find_state {
  unsigned char target[32];
  dht_node_t *current;
};

static int
_find_walker(void *ctx, dht_bucket_t *root){
  struct _find_state *state = *ctx;
  unsigned char adelta[32], bdelta[32];

  for(int i = 0; i < root->length; i++){
    dht_xor(adelta, state->target, root->nodes[i]->id);
    dht_xor(bdelta, state->target, state->current);

    if(dht_compare(adelta, bdelta) == -1)
      state->current = root->nodes[i];
  }

  return 0;
}

dht_node_t *
dht_find_node(dht_t *dht, unsigned char key[32]) {
  if(dht->bucket->length == 0) return NULL;

  struct _find_state state;

  state.current = dht->bucket->nodes[0];
  state.target = key;

  dht_bucket_walk((void *) &min, dht->root, _find_walker);

  return min;
}
