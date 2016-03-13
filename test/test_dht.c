#include "../dht.c"
#include "tap.h"

static void
test_init(){
  dht_t *dht = dht_new(9999);
  ok(dht != NULL, "dht allocated correctly");
  dht_close(dht);
}

int
main(){
  start_test;
  test_init();
}
