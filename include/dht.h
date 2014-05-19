#ifndef DHT_H_
#define DHT_H_

struct dht_bucket_t;
struct dht_queue_t;



typedef struct dht_t {
  struct dht_queue_t *queue;
  struct dht_bucket_t *bucket;
  int (*send)(struct dht_t *dht, dht_op_t op, char *data, int length, unsigned char node_id[32]);
} dht_t;

dht_t *
dht_new();

int
dht_recv(dht_t *dht, dht_op_t op, char *data, int length, unsigned char node_id[32]);

typedef void
(*dht_get_callback)(void *ctx, dht_node_t *node, int length);

int
dht_get(dht_t *dht, unsigned char *key, dht_get_callback cb);

void
dht_free(dht_t *table);

#endif