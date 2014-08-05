#include <string.h>
#include <stdlib.h>
#include <stdio.h>

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

  action mark {
    s = p;
  }

  action node {
    cur->next = calloc(1, sizeof(dht_proto_t));

    if(cur->next == NULL){
      dht_proto_free(proto);
      return NULL;
    }

    if(s + 32 > pe) fgoto protocol_error;

    strncpy(cur->val.node_id, s + 1, 32);
    cur = cur->next;
  }

  action protocol_error {
    puts("error"); return NULL;
  }

  ping = DHT_PING @ping >mark;
  find_nodes = DHT_FIND_NODES @find_nodes >mark %node;

  Protocol = ping | find_nodes;

  main := Protocol $! ;
}%%

%% write data;

dht_proto_t *
parse(const char *data, const int length){
  char *p  = (char *) data;
  char *pe = p + length;

  char *eof = pe;
  char *s  = p;
  int cs   = 0;

  dht_proto_t *proto = calloc(1, sizeof(dht_proto_t));
  if(!proto)
    return NULL;

  dht_proto_t *cur = proto;

  %% write init;
  %% write exec;

  return proto;
}

int
encode(char **ret, const dht_proto_t *proto){
  switch(proto->op){
    case DHT_PING:
      *ret = calloc(1, sizeof(char));
      *ret[0] = DHT_PING;
      break;
    default:
      asprintf(ret, "err");
  }
  return 0;
}

void
dht_proto_free(dht_proto_t *proto){
  dht_proto_t *cur = proto;
  while((cur = cur->next))
    free(cur);
  free(proto);
}
