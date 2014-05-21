#ifndef DHT_BUCKET_H_
#define DHT_BUCKET_H_

#include <stdint.h>
#include "dht_node.h"

typedef struct dht_bucket_t {
  dht_node_t* nodes[8];
  int length;
  uint8_t upper_limit;
  uint8_t lower_limit;
  struct dht_bucket_t *next;
} dht_bucket_t;

dht_bucket_t*
dht_bucket_new(uint8_t upper, uint8_t lower);

dht_bucket_t*
dht_bucket_insert(dht_bucket_t *root, dht_node_t *node);

typedef int (*dht_bucket_walk_callback)(void *ctx, dht_bucket_t *root);

void
dht_bucket_walk(void *ctx, dht_bucket_t *root, dht_bucket_walk_callback cb);

void
dht_bucket_update(dht_bucket_t *root);

void
dht_bucket_free(dht_bucket_t *root);

#endif
