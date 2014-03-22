#ifndef DHT_BUCKET_H_
#define DHT_BUCKET_H_

#include "dht_node.h"

typedef struct dht_bucket_t {
  dht_node_t[8] nodes;
  int length;
  uint8_t upper_limit;
  uint8_t lower_limit; // make these a char array
  dht_bucket_t *lower;
  dht_bucket_t *upper;
} dht_bucket_t;

dht_bucket_t*
dht_bucket_new(char *upper, char *lower);

dht_bucket_t*
dht_bucket_insert(dht_bucket_t *root, dht_node_t *node);

void
dht_bucket_walk(dht_bucket_t *root, dht_baton_t *baton);

void
dht_bucket_update(dht_bucket_t *root);

void
dht_bucket_free(dht_bucket_t *root);

#endif