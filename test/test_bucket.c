#include "util.h"
#include "../dht.c"
#include "tap.h"

static void
test_bucket(){
  bucket_t *bucket = bucket_new(0, 255);
  ok(bucket != NULL, "bucket is not null");
  bucket_free(bucket);
}


static int
_walker(void *ctx, bucket_t *root){
  (void) root;
  int *i = ctx;
  (*i) += root->length;
  return 0;
}

static void
test_bucket_insert(){
  bucket_t *bucket = bucket_new(0, 255);
  ok(bucket != NULL, "bucket is not null");
  int ins = 0;
  struct sockaddr_storage st;
  for(int i = 0; i < 2048; i++){
    uint8_t buf[32];
    random_bytes(buf, 32);
    node_t *node = node_new(buf, &st);
    bucket_t *nins = bucket_insert(bucket, node);
    if(!nins) {
      node_free(node);
    } else {
      ins++;
    }
  }
  int j = 0;
  bucket_walk(&j, bucket, _walker);
  bucket_free(bucket);
  ok(j == ins, "right number of nodes inserted");
}

int
main(){
  start_test;
  test_bucket();
  test_bucket_insert();
}
