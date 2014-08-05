#include <string.h>

#include "util.h"
#include "dht_protocol.h"
#include "tap.h"

static void
test_ping(){
  dht_proto_t proto = {
    .op = DHT_PING
  };
  char *buf;
  encode(&buf, &proto);

  dht_proto_t *ping = parse((const char *) buf, 1);

  ok(ping->op == proto.op, "got a ping");

  free(buf);
  dht_proto_free(ping);
}

// static void
// test_find_nodes(){
//   dht_proto_t proto = {
//     .op = DHT_PING,
//     .val.node_id = "Ever tried Ever failed No matter"
//   };

//   char *buf;
//   encode(&buf, &proto);
//   printf("%s", buf + 1);
//   dht_proto_t *nodes = parse((const char *) buf, 33);

//   ok(nodes->op == proto.op, "got find_nodes");
//   ok(strncmp(nodes->val.node_id, "Ever tried Ever failed No matter", 32) == 0, "right node id");
//   dht_proto_free(nodes);
// }

int
main(){
  start_test;
  test_ping();
  // test_find_nodes();
  return 0;
}
