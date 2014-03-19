#ifndef DHT_NODE_H_
#define DHT_NODE_H_

typedef struct {
  char[160] id;
} dht_node_t;

dht_node_t *
dht_node_new();

#endif