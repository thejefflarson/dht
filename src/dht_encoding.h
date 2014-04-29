#ifndef DHT_ENCODE_H_
#define DHT_ENCODE_H_

typedef enum {
  DHT_BE_STR,
  DHT_BE_INT,
  DHT_BE_LIST,
  DHT_BE_DICT
} dht_be_type;

typedef struct {
  char *key;
  struct dht_be_node *val;
} dht_be_dict;

typedef struct dht_be_node {
  dht_be_type type;
  union {
    long long i;
    char* str;
    dht_be_node **list;
    dht_be_dict *dict;
  } val;
} dht_be_node;

dht_be_node *
dht_be_decode(char *str, long long size);

char *
dht_be_encode(dht_be_node *node);

void
dht_be_free(dht_be_node *node);

#endif