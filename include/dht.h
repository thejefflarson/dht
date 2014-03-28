#ifndef DHT_H_
#define DHT_H_

typedef struct dht_queue_t;
typedef struct dht_bucket_t;

typedef struct {
  dht_queue_t *queue;
  dht_bucket_t *bucket;
} dht_t;

typedef enum {
  DHT_PING       = 100,
  DHT_STORE      = 200,
  DHT_FIND_NODE  = 300,
  DHT_FIND_VALUE = 400
} dht_op_t;

dht_t *
dht_new();

typedef void
(*dht_set_callback)(void *ctx, char *key, char *data, int length);

int
dht_set(dht_t *dht, char *key, char *data, int length, dht_set_callback *cb);

typedef void
(*dht_get_callback)(void *ctx, char *key, char *data, int length);

int
dht_get(dht_t *dht, char *key, *dht_set_callback cb);

int
dht_receive(dht_t *dht, dht_op_t op, char *data, int length, char[32] node_id);

int
dht_send(dht_t *dht, dht_op_t op, char *data, int length, char[32] node_id);

void
dht_free(dht_t *table);

#endif