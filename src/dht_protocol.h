#ifndef DHT_NODE_H_
#define DHT_NODE_H_

typedef enum {
  DHT_PING=100,
  DHT_PONG=200,
  DHT_FIND_NODES=300,
  DHT_NODES=400,
  DHT_STORE=500,
  DHT_FIND_VALUE=600,
  DHT_VALUE=700
} dht_op_t;

dht_proto *
parse(char *data, int length);

#endif
