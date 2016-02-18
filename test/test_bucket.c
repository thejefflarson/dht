#include "util.h"
#include "dht_bucket.h"
#include "tap.h"

static void
test_bucket(){
  dht_bucket_t *bucket = dht_bucket_new(0, 255);
  ok(bucket != NULL, "bucket is not null");
  dht_bucket_free(bucket);
}


static int
_walker(void *ctx, dht_bucket_t *root){
  (void) root;
  int *i = ctx;
  (*i) += root->length;
  return 0;
}

static void
test_bucket_insert(){
  dht_bucket_t *bucket = dht_bucket_new(0, 255);
  ok(bucket != NULL, "bucket is not null");
  int ins = 0;
  struct sockaddr_storage st;
  for(int i = 0; i < 2048; i++){
    uint8_t buf[32];
    random_bytes(buf, 32);
    dht_node_t *node = dht_node_new(buf, &st);
    dht_bucket_t *nins = dht_bucket_insert(bucket, node);
    if(!nins) {
      dht_node_free(node);
    } else {
      ins++;
    }
  }
  int j = 0;
  dht_bucket_walk(&j, bucket, _walker);
  dht_bucket_free(bucket);
  ok(j == ins, "right number of nodes inserted");
}

int
main(){
  start_test;
  test_bucket();
  test_bucket_insert();
}
