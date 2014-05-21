#include "util.h"
#include "dht_bucket.h"
#include "tap.h"

static void
test_find_node(){
  dht_t *dht = dht_new();
  ok(dht != NULL, "dht allocated correctly");
  // todo
  dht_free(dht);
}

void
main(){
  start_test;
  test_ping();
}