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

  // dht->queue = (dht_queue_t *) dht_queue_new();
  // if(dht->queue == NULL){
  //   dht_bucket_free(dht->bucket);
  //   free(dht);
  //   return NULL;
  // }

  return dht;
}

int
dht_set(dht_t *dht, char *key, char *data, int length, dht_set_callback *cb){
  return 0;
}

int
dht_get(dht_t *dht, char *key, dht_get_callback cb) {
  return 0;
}

void
dht_free(dht_t *table) {

}

// private functions here for now
int
dht_ping(dht_node_t* node) {
  return 0;
}

int
dht_find_node(char *key) {
  return 0;
}
