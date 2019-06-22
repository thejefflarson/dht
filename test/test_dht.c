#include "../dht.c"
#include "tap.h"
#include <arpa/inet.h>
#include <assert.h>

static void
test_init(){
  dht_t *dht = dht_new(9999);
  ok(dht != NULL, "dht allocated correctly");
  dht_t *dht2 = dht_new(9999);
  ok(dht2 == NULL, "dht errors on open port");
  dht_close(dht);
}

static int ran = 0;
static int lkp = 0;
static int err = 0;
static uint8_t key[DHT_HASH_SIZE] = {0};
static uint8_t data[] = "hello";

static ssize_t
lookup(const uint8_t skey[DHT_HASH_SIZE], void **sdata) {
  ok(crypto_verify_32(skey, key) == 0, "key matches");
  *sdata = data;
  lkp++;
  return sizeof(data);
}

static int
store(const uint8_t skey[DHT_HASH_SIZE], const void *sdata, const size_t length) {
  ok(length == sizeof(data), "length matches in store");
  ok(memcmp(sdata, data, length) == 0, "length matches in store");
  ok(crypto_verify_32(skey, key) == 0, "key matches in store");
  return sizeof(data);
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
  err++;
}

static void
test_set_get() {
  dht_t *dht  = dht_new(9999);
  dht_t *dht2 = dht_new(10001);
  ok(dht != NULL, "dht allocated correctly");
  ok(dht2 != NULL, "multiple dhts per process");
  int ret = blake2(key, data, NULL, DHT_HASH_SIZE, sizeof(data), 0);
  ok(ret == 0, "hashed 'hello' correctly");

  struct sockaddr_storage addr = {0};
  socklen_t slen = sizeof(addr);
  ret = getsockname(dht2->socket, (struct sockaddr *)&addr, &slen);
  ok(ret == 0, "parsed address correctly");
  dht_add_node(dht, dht2->id, &addr);
  dht_set_storage(dht2, store, lookup);
  ret = dht_set(dht, data, sizeof(data), success, error, NULL);
  if(ret == -1) {
    printf("%s, %s: %i\n", strerror(errno), __FILE__, __LINE__);
    fflush(stdout);
  }
  ok(ret >= 0, "sent a set request");
  ret = dht_run(dht2, 100);
  ok(ret == 0, "dht2 received a set request");
  ret = dht_run(dht, 100);
  ok(ret == 0, "dht received set response");
  dht_get(dht, key, success, error, NULL);
  ret = dht_run(dht2, 100);
  ok(ret == 0, "dht2 received a get request");
  ret = dht_run(dht, 100);
  ok(ret == 0, "dht received get response");

  dht_close(dht);
  dht_close(dht2);
  ok(ran == 1, "got a value");
  ok(lkp == 1, "stored the value");
  ok(err == 0, "couldn't find a value");
}

typedef struct {
  uint8_t hash[DHT_HASH_SIZE];
  uint8_t value;
  bool have;
} state_t;

#define DHTS 20
#define PER 20
dht_t *dhts[DHTS];

state_t states[DHTS][PER];

void
got(void *closure, uint8_t key[DHT_HASH_SIZE], uint8_t *data, size_t length) {
  (void) closure, (void) key, (void) length;
  printf("# Got %i !\n", (int) *data);
  size_t num = *(size_t *)data;
  for(int i = 0; i < DHTS; i++) {
    if(!states[i][num].have) {
      states[i][num].have = true;
    }
  }
}

void
failed(void *closure) {
  state_t *state = closure;
  printf("# failed on %i", state->value);
  ok(false, "failed lookup");
}

int
set(const uint8_t key[DHT_HASH_SIZE], const void *data, const size_t length) {
  printf("# set %i!\n", *(int*) data);
  (void) key, (void) length;
  return 0;
}

ssize_t
find(const uint8_t key[DHT_HASH_SIZE], void **data) {
  uint8_t h[DHT_HASH_SIZE] = {0};
  int i;

  for(i = 0; i < PER; i++) {
    int ret = blake2(h, &i, NULL, DHT_HASH_SIZE, sizeof(i), 0);
    if(ret == 0 && crypto_verify_32(h, key) == 0) {
      *data = calloc(1, sizeof(i));
      int *j = *data;
      *j = i;
      return (double)rand() / RAND_MAX > 0.3 ? sizeof(i) : -1;
    }
  }
  return -1;
}

static void
test_full_network() {
  for(size_t i = 0; i < DHTS; i++) {
    dhts[i] = dht_new(10000 + i);
    dht_set_storage(dhts[i], set, find);
    ok(dhts[i], "allocate a dht");
    if(i > 0) {
      struct sockaddr_storage addr = {0};
      socklen_t slen = sizeof(addr);
      int ret = getsockname(dhts[i-1]->socket,(struct sockaddr *)&addr, &slen);
      ok(ret == 0, "parsed address correctly");
      dht_add_node(dhts[i], dhts[i-1]->id, &addr);
    }
    dht_set_storage(dhts[i], set, find);
  }

  for(int i = 0; i < DHTS; i++) {
    for(int j = 0; j < PER; j++) {
      states[i][j].value = j;
      int ret = blake2(states[i][j].hash, &j, NULL, DHT_HASH_SIZE, sizeof(j), 0);
      if(i == j) states[DHTS - 1][PER - 1].have = true;
      assert(ret != -1);
    }
  }

  while(1) {
    bool complete = true;
    for(int i = 0; i < DHTS; i++) {
      dht_run(dhts[i], 10);
      for(int j = 0; j < PER; j++) {
        complete = complete && states[i][j].have;
        if(!states[i][j].have) {
          dht_get(dhts[i], states[i][j].hash, got, failed, &states[i][j]);
        }
      }
    }
    if(complete) break;
  }

  for(int i = 0; i < DHTS; i++) {
    printf("# dht[%i] peers: %i\n", i, dhts[i]->bucket->length);
    dht_close(dhts[i]);
  }
}
#undef DHTS
#undef PER
int
main(){
  start_test;
  test_init();
  test_set_get();
  test_full_network();
}
