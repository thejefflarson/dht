#ifndef _DHT_H_
#define _DHT_H_
#include <stddef.h>
#include <stdint.h>
#include <sys/socket.h>
#include <unistd.h>

#define DHT_HASH_SIZE 32

typedef struct dht_s dht_t;

// sets errno on error returns NULL
dht_t *
dht_new(int port);

typedef void
(*dht_get_callback)(void *closure, uint8_t key[DHT_HASH_SIZE], uint8_t *data, size_t length);

typedef void
(*dht_failure_callback)(void *closure);

int
dht_get(dht_t *dht, uint8_t key[DHT_HASH_SIZE],
        dht_get_callback success, dht_failure_callback error, void *closure);

int
dht_set(dht_t *dht, void *data, size_t len,
        dht_get_callback success, dht_failure_callback error, void *closure);

int
dht_run(dht_t *dht, int timeout);

void
dht_close(dht_t *dht);

int
dht_add_node(dht_t *dht, uint8_t key[DHT_HASH_SIZE], struct sockaddr_storage *addr);

typedef int
(*dht_store_callback)(const uint8_t key[DHT_HASH_SIZE], const void *data, const size_t length);

typedef ssize_t
(*dht_lookup_callback)(const uint8_t key[DHT_HASH_SIZE], void **data);

void
dht_set_storage(dht_t *dht, dht_store_callback store, dht_lookup_callback lookup);
#endif
