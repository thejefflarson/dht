#include <stdlib.h>
#include "dht.h"
#include "dht_node.h"
#include "dht_bucket.h"

#define DHT_TOKEN_LENGTH 32


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

// private functions here for now
void
dht_ping(dht_t *table, dht_node_t* node) {
  table->send(dht, DHT_PING, NULL, 0, node->id);
}


static int
_find_walker(void *ctx, dht_bucket_t *root){
  dht_node_t* min = *ctx;

  for(int i = 0; i < root->length; i++){
    if(dht_compare(min, &root->nodes[i]) == -1){
      ctx = &root->nodes[i];
    }
  }

  return 0;
}

dht_node_t *
dht_find_node(dht_t *table, char *key) {
  if(table->bucket->length == 0) return NULL;

  dht_node_t *min = table->bucket->nodes[0];

  dht_bucket_walk((void *) &min, table->root, );

  return min;
}
