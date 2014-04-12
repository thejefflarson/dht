#include <memory.h>
#include "dht.h"
#include "dht_node.h"

dht_t *
dht_new(){
  dht_t *dht = (dht_t *) malloc(sizeof(dht_t));
  if(dht == NULL) return NULL;

  dht->bucket = (dht_bucket_t *) dht_bucket_new();
  if(dht->bucket == NULL) {
    free(dht);
    return NULL;
  }

  dht->queue = (dht_queue_t *) dht_queue_new();
  if(dht->queue == NULL){
    dht_bucket_free(dht->bucket);
    free(dht);
    return NULL;
  }

  return dht;
}

int
dht_set(char *key, char *value, size_t size, dht_baton_t *baton){
  return 0;
}

int
dht_get(char *key, char **buf, dht_baton_t *baton) {
  return 0;
}

int
dht_receive(dht_t *dht, dht_op_t op, char *args) {
  return 0;
}

int
dht_send(dht_t *dht, dht_send_baton_t *baton) {
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
