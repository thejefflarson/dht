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
dht_free(dht_t *table) {
  free(table)
}

void
dht_ping(dht_t *table, dht_node_t* node) {
  table->send(dht, DHT_PING, NULL, 0, node->id);
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
dht_find_node(dht_t *table, unsigned char key[32]) {
  if(table->bucket->length == 0) return NULL;

  struct _find_state state;

  state.current = table->bucket->nodes[0];
  state.target = key;

  dht_bucket_walk((void *) &min, table->root, _find_walker);

  return min;
}
