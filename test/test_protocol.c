#include "util.h"
#include "dht_protocol.h"
#include "tap.h"

static void
test_find_nodes(){

  ok(proto != NULL);

  dht_proto_free(proto);
}

void
main(){
  start_test;
  test_find_nodes();
}