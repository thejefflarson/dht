#ifndef _DHT_H_
#define _DHT_H_
#include <stddef.h>
#include <stdint.h>

#define DHT_HASH_SIZE 32

typedef struct dht_s dht_t;

dht_t *
dht_new();

typedef void
(*dht_get_callback)(void *closure, uint8_t node_id[DHT_HASH_SIZE], uint8_t *data, size_t length);

int
dht_get(dht_t *dht, uint8_t *key, dht_get_callback cb, void *closure);

int
dht_set(dht_t *dht, void *data, size_t len);

int
dht_run(dht_t *dht);

void
dht_free(dht_t *dht);
#endif
