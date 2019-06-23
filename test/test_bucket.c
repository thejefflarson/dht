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
  randombytes(a, DHT_HASH_SIZE);
  ok(bucket != NULL, "bucket is not null");
  int ins = 0;
  struct sockaddr_storage st = {0};
  node_t *node;
  for(int i = 0; i < 100; i++){
    uint8_t buf[DHT_HASH_SIZE];
    randombytes(buf, DHT_HASH_SIZE);
    node = node_new(buf, &st);
    bucket_t *nins = bucket_insert(bucket, node, a);
    if(!nins) {
      node_free(node);
    } else {
      ins++;
    }
  }
  node_t *nodes[8];
  for (bucket_t *root = bucket; root && root->length > 0; root = root->next)
    node = root->nodes[0];
  find_nodes(nodes, bucket, node->id);
  ok(nodes[0] == node, "found the right node");
  int j = 0;
  bucket_walk(&j, bucket, _walker);
  bucket_free(bucket);
  ok(j == ins, "right number of nodes inserted");
  printf("# %i\n", ins);
  ok(ins > 80, "majority of nodes inserted");
}

int
main(){
  start_test;
  test_bucket();
  test_bucket_insert();
}
