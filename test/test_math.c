#include "../dht.c"
#include "tap.h"

static void
test_addition() {
  uint8_t a[DHT_HASH_SIZE] = {0};
  uint8_t b[DHT_HASH_SIZE] = {0};
  uint8_t c[DHT_HASH_SIZE] = {0};
  a[DHT_HASH_SIZE - 1] = 1;
  b[DHT_HASH_SIZE - 1] = 2;
  int ret = add_ids(a, b, c);
  ok(ret == 0, "adds two numbers");
  ok(c[DHT_HASH_SIZE - 1] == 3, "1 + 2 = 3");
  b[DHT_HASH_SIZE - 1] = 255;
  ret = add_ids(a, b, c);
  ok(ret == 0, "add with carry");
  ok(c[DHT_HASH_SIZE - 2] == 1 && c[DHT_HASH_SIZE - 1] == 0, "255 + 1 = 256");
  a[DHT_HASH_SIZE - 1] = 255;
  a[DHT_HASH_SIZE - 2] = 255;
  b[DHT_HASH_SIZE - 1] = 2;
  ret = add_ids(a, b, c);
  ok(ret == 0, "multiple significant digits");
  ok(c[DHT_HASH_SIZE - 3] == 1 && c[DHT_HASH_SIZE - 1] == 1, "65535 + 2 = 65537");
  memset(a, 0xFF, sizeof(a));
  ret = add_ids(a, b, c);
  ok(ret == -1, "reports overflow");
}

static void
test_subtraction(){
  uint8_t b[DHT_HASH_SIZE] = {0};
  uint8_t a[DHT_HASH_SIZE] = {0};
  uint8_t c[DHT_HASH_SIZE] = {0};
  a[DHT_HASH_SIZE - 1] = 2;
  b[DHT_HASH_SIZE - 1] = 1;
  int ret = subtract_ids(a, b, c);
  ok(ret == 0, "subtracts two numbers");
  ok(c[DHT_HASH_SIZE - 1] == 1, "2 - 1 = 1");
  memset(a, 0, sizeof(a));
  a[DHT_HASH_SIZE - 2] = 1;
  ret = subtract_ids(a, b, c);
  ok(ret == 0, "subtracts with carry");
  ok(c[DHT_HASH_SIZE - 1] == 255 && c[DHT_HASH_SIZE - 2] == 0, "256 - 1 = 255");
  a[DHT_HASH_SIZE - 1] = 0;
  a[DHT_HASH_SIZE - 2] = 0;
  a[DHT_HASH_SIZE - 3] = 1;
  b[DHT_HASH_SIZE - 1] = 1;
  ret = subtract_ids(a, b, c);
  ok(ret == 0, "carries accross multiple significant digits");
  ok(c[DHT_HASH_SIZE - 1] == 255 && c[DHT_HASH_SIZE  -2] == 255, "131072 - 1 = 131071");
  memset(a, 0, sizeof(a));
  ret = subtract_ids(a, b, c);
  ok(ret == -1, "reports underflow");
}

static void
test_division() {
  uint8_t a[DHT_HASH_SIZE] = {0};
  uint8_t b[DHT_HASH_SIZE] = {0};
  a[DHT_HASH_SIZE - 1] = 2;
  divide_by_two(a, b);
  ok(b[DHT_HASH_SIZE - 1] == 1, "2 / 2 = 1");
  memset(a, 0, sizeof(a));
  a[DHT_HASH_SIZE - 2] = 1;
  divide_by_two(a, b);
  ok(b[DHT_HASH_SIZE - 1] == 0x80, "256 / 2 = 128");
}

int
main(){
  start_test;
  test_addition();
  test_subtraction();
  test_division();
}
