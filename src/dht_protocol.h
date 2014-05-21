#ifndef DHT_NODE_H_
#define DHT_NODE_H_

typedef enum {
  DHT_PING=10,
  DHT_PONG=20,
  DHT_FIND_NODES=30,
  DHT_NODES=40,
  DHT_STORE=50,
  DHT_FIND_VALUE=60,
  DHT_VALUE=70
} dht_op_t;

typedef struct dht_proto {
  dht_op_t op;
  union {
    char node_id[32];
  } val;
  struct dht_proto *next;
} dht_proto_t;

dht_proto_t *
parse(char *data, int length);

void
dht_proto_free(dht_proto_t *);

#endif
