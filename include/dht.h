#pragma once
#include <stddef.h>
#include <stdint.h>

struct dht_bucket_t;
struct dht_queue_t;
#define DHT_HASH_SIZE 32

typedef enum {
  DHT_SEARCH
} dht_op_t;

typedef struct dht_t {
  struct dht_queue_t *queue;
  struct dht_bucket_t *bucket;
  int (*send)(struct dht_t *dht, dht_op_t op, uint8_t *data, size_t length, uint8_t node_id[DHT_HASH_SIZE]);
} dht_t;

dht_t *
dht_new();

int
dht_run(dht_t *dht, uint8_t *data, size_t length, struct sockaddr from, size_t from_length);

typedef void
(*dht_get_callback)(void *closure, uint8_t node_id[DHT_HASH_SIZE], uint8_t *data, size_t length);

int
dht_get(dht_t *dht, uint8_t *key, dht_get_callback cb, void *closure);

int
dht_set(dht_t *dht, void *data, size_t len);

void
dht_free(dht_t *table);
