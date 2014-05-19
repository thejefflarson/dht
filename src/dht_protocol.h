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

typedef dh


dht_proto_t *
parse(char *data, int length);

#endif
