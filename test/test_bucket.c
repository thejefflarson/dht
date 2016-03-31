#include "../dht.c"
#include "tap.h"

static void
test_bucket(){
  uint8_t a[DHT_HASH_SIZE] = {0xFF};
  bucket_t *bucket = bucket_new(a);
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
  uint8_t a[DHT_HASH_SIZE] = {0xFF};
  bucket_t *bucket = bucket_new(a);
  ok(bucket != NULL, "bucket is not null");
  int ins = 0;
  struct sockaddr_storage st = {0};
  node_t *node;
  for(int i = 0; i < 100; i++){
    uint8_t buf[DHT_HASH_SIZE];
    randombytes(buf, DHT_HASH_SIZE);
    node = node_new(buf, &st);
    bucket_t *nins = bucket_insert(bucket, node);
    if(!nins) {
      node_free(node);
    } else {
      ins++;
    }
  }
  node_t *nodes[8];
  find_nodes(nodes, bucket, node->id);
  ok(nodes[0] == node, "found the right node");
  int j = 0;
  bucket_walk(&j, bucket, _walker);
  bucket_free(bucket);
  ok(j == ins, "right number of nodes inserted");
  ok(ins > 80, "majority of nodes inserted");
}

static void
test_bucket_update(){
  uint8_t a[DHT_HASH_SIZE] = {0xFF};
  bucket_t *bucket = bucket_new(a);
  struct sockaddr_storage st = {0};
  int j = 0;
  uint8_t buf[DHT_HASH_SIZE];
  randombytes(buf, DHT_HASH_SIZE);
  node_t *node = node_new(buf, &st);
  bucket_insert(bucket, node);
  node->last_heard = 0;
  bucket_update(bucket);
  bucket_walk(&j, bucket, _walker);
  ok(j == 0, "old nodes should be removed");
  bucket_free(bucket);
}

int
main(){
  start_test;
  test_bucket();
  test_bucket_insert();
  test_bucket_update();
}
