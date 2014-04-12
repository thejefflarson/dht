#ifndef DHT_BUCKET_H_
#define DHT_BUCKET_H_

#include "dht_node.h"

typedef struct dht_bucket_t {
  dht_node_t[8] nodes;
  int length;
  uint8_t upper_limit;
  uint8_t lower_limit; // make these a char array
  dht_bucket_t *next;
} dht_bucket_t;

dht_bucket_t*
dht_bucket_new(char *upper, char *lower);

dht_bucket_t*
dht_bucket_insert(dht_bucket_t *root, dht_node_t *node);

typedef int (*dht_bucket_walk_callback)(void *ctx, dht_bucket_t *root);

void
dht_bucket_walk(void *ctx, dht_bucket_t *root, dht_bucket_walk_callback *cb);

void
dht_bucket_update(dht_bucket_t *root);

void
dht_bucket_free(dht_bucket_t *root);

#endif