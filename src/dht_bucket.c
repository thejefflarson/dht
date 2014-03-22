#include <stdlib.h>
#include "dht_bucket.h"

dht_bucket_t*
dht_bucket_new() {
  dht_bucket_t* bucket;
  if((bucket = malloc(sizeof(dht_bucket_t))) == NULL)
    return NULL;
  return bucket;
}

dht_bucket_t*
dht_bucket_insert(dht_bucket_t *root, dht_node_t *node) {

}

void
dht_bucket_walk(dht_bucket_t *root, dht_baton_t *baton) {

}

void
dht_bucket_free(dht_bucket_t *root) {

}
