#include "../dht.c"
#include "tap.h"

static void
test_init(){
  dht_t *dht = dht_new(9999);
  ok(dht != NULL, "dht allocated correctly");
  dht_close(dht);
}

static int ran = 0;

static void
lookup() {
  
}

static void
success(void *closure, uint8_t key[DHT_HASH_SIZE], uint8_t *data, size_t length) {
  (void) closure, (void) key, (void) data, (void) length;
  ok(true, "found a key");
  ran++;
}

static void
error(void *closure) {
  (void)closure;
  ok(false, "error finding something");
  ran++;
}

static void
test_set_get() {
  dht_t *dht  = dht_new(9999);
  dht_t *dht2 = dht_new(10000);
  ok(dht != NULL, "dht allocated correctly");
  ok(dht2 != NULL, "multiple dht's per process");
  char data[] = "hello";
  uint8_t key[DHT_HASH_SIZE] = {0};
  int ret = blake2(key, data, NULL, DHT_HASH_SIZE, sizeof(data), 0);
  ok(ret == 0, "hashed 'hello' correctly");
  struct sockaddr_storage addr;
  dht_insert(dht, dht2->id, addr);
  dht_set(dht, data, sizeof(data), success, error, NULL);
  dht_run(dht2, 10);
  dht_get(dht, key, success, error, NULL);
  dht_close(dht);
  dht_close(dht2);
  assert(ran == 2, "got a value");
}

int
main(){
  start_test;
  test_init();
  test_set_get();
}
