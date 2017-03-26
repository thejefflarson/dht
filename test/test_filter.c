#include <inttypes.h>
#include "../dht.c"
#include "tap.h"

static void
out(uint8_t o[FILTER_SIZE]) {
  for(int i = 0; i < FILTER_SIZE; i++) {
    printf("%02X ", o[i]);
  }
  puts("");
}

static void
test_filter() {
  uint8_t filter[FILTER_SIZE] = {0};
  uint8_t hash[DHT_HASH_SIZE] = {0};
  randombytes(hash, DHT_HASH_SIZE);
  filter_add(filter, hash);
  ok(filter_includes(filter, hash), "bloom filter matches a key");
  randombytes(hash, DHT_HASH_SIZE);
  ok(!filter_includes(filter, hash), "bloom filter doesn't have unknown key");

  for(int i = 0; i < 100; i++) {
    randombytes(hash, DHT_HASH_SIZE);
    filter_add(filter, hash);
    ok(filter_includes(filter, hash), "filter has key.");
  }

  float fp = 0;
  for(int i = 100; i < 10000; i++) {
    randombytes(hash, DHT_HASH_SIZE);
    if(filter_includes(filter, hash))
      fp++;
  }
  printf("# False positive rate: %.2f%%\n# filter: ", fp / 10000.0 * 100);
  out(filter);
}

int
main() {
  start_test;
  test_filter();
}
