#ifndef _DHT_H_
#define _DHT_H_
#include <stddef.h>
#include <stdint.h>

#define DHT_HASH_SIZE 32

typedef struct dht_s dht_t;

// sets errno on error returns -1 for general errors and -2 for getaddrinfo errors
int
dht_init(dht_t *dht, int port);

typedef void
(*dht_get_callback)(void *closure, uint8_t node_id[DHT_HASH_SIZE], uint8_t *data, size_t length);

typedef void
(*dht_failure_callback)(void *closure);

int
dht_get(dht_t *dht, uint8_t key[DHT_HASH_SIZE], dht_get_callback success, dht_failure_callback error, void *closure);

int
dht_set(dht_t *dht, void *data, size_t len);

int
dht_run(dht_t *dht);

void
dht_close(dht_t *dht);
#endif
