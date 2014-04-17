#ifndef DHT_NODE_H_
#define DHT_NODE_H_
#include <time.h>
#include <stdboo.h>

typedef struct dh_node_t {
  char id[32];
  time_t created_at;
  time_t last_heard;
} dht_node_t;

dht_node_t *
dht_node_new(char[32]);


bool
dht_node_good(dht_node_t *);

void
dht_node_free(dht_node_t*);

#endif