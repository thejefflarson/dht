#ifndef DHT_H_
#define DHT_H_

typedef struct dht_queue_t;
typedef struct dht_bucket_t;

typedef struct {
  dht_queue_t *queue;
  dht_bucket_t *bucket;
} dht_t;

typedef struct {
  void *user_data;
  void (*callback)(dht_baton_t* baton);
  dht_op_t op;
  char[160] node_id;
  char *data;
  size_t size;
} dht_baton_t;

typedef enum {
  DHT_PING       = 100,
  DHT_STORE      = 200,
  DHT_FIND_NODE  = 300,
  DHT_FIND_VALUE = 400
} dht_op_t;

dht_t *
dht_new();

int
dht_set(dht_t *dht, char *key, dht_baton_t *baton);

int
dht_get(dht_t *dht, char *key, dht_baton_t *baton);

int
dht_receive(dht_t *dht, dht_baton_t *baton);

int
dht_send(dht_t *dht, dht_baton_t *baton);

void
dht_free(dht_t *table);

#undef DHT_BATON_FIELDS
#endif