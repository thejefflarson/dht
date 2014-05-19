#include <string.h>

#include "dht_protocol.h"

%%{
  machine Protocol;

  import "dht_protocol.h";

  action ping {
    proto->op = DHT_PING;
  }

  action find_nodes {
    proto->op = DHT_FIND_NODES;
  }

  action start_node {
    s = p;
  }

  action node {
    cur->next = calloc(1, sizeof(dht_proto_t));

    if(cur->next == NULL){
      dht_proto_free(proto);
      return NULL;
    }

    cur = cur->next;
    strndup(cur->val.id, s, 32);
  }

  node = any{32} >start_node %node;

  ping = DHT_PING @ping;
  find_nodes = DHT_FIND_NODES @find_nodes node;

  Protocol = ping | find_nodes;

  main := Protocol;
}%%

%% write data;

dht_proto_t *
parse(char *data, int length){
  char *p  = data;
  char *pe = p + length;
  char *s  = p;
  int cs   = 0;

  dht_proto_t *proto = calloc(1, sizeof(dht_proto_t));
  if(!proto)
    return NULL;
  dht_proto_t *cur = proto;

  %% write exec;

  return proto;
}

char *
encode(dht_proto_t *proto){
  return "";
}