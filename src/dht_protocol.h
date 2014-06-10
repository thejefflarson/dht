#ifndef DHT_PROTOCOL_H_
#define DHT_PROTOCOL_H_

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
parse(const char *data, const int length);

int
encode(char **ret, const dht_proto_t *proto);

void
dht_proto_free(dht_proto_t *);

#endif
