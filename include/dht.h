#ifndef DHT_H_
#define DHT_H_

struct dht_bucket_t;
struct dht_queue_t;

typedef enum {
  DHT_PING       = 100,
  DHT_STORE      = 200,
  DHT_FIND_NODE  = 300,
  DHT_FIND_VALUE = 400
} dht_op_t;

typedef struct dht_t {
  struct dht_queue_t *queue;
  struct dht_bucket_t *bucket;
  int (*dht_recv)(struct dht_t *dht, dht_op_t op, char *data, int length, char node_id[32]);
  int (*dht_send)(struct dht_t *dht, dht_op_t op, char *data, int length, char node_id[32]);
} dht_t;

dht_t *
dht_new();

typedef void
(*dht_set_callback)(void *ctx, char *key, char *data, int length);

int
dht_set(dht_t *dht, char *key, char *data, int length, dht_set_callback *cb);

typedef void
(*dht_get_callback)(void *ctx, char *key, char *data, int length);

int
dht_get(dht_t *dht, char *key, dht_get_callback cb);



void
dht_free(dht_t *table);

#endif