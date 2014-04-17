#include <assert.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include "dht_bucket.h"

void
random_bytes(char *buf, size_t size){
  int f = open("/dev/urandom", O_RDONLY);
  if(!f) exit(1);
  read(f, buf, size);
  close(f);
}


void
test_bucket(){
  dht_bucket_t *bucket = dht_bucket_new(0, 255);
  assert(bucket != NULL);
  dht_bucket_free(bucket);
}


int
_walker(void *ctx, dht_bucket_t *root){
  (void) root;
  int *i = ctx;
  (*i)++;
  return 1;
}

void
test_bucket_insert(){
  dht_bucket_t *bucket = dht_bucket_new(0, 255);
  for(int i = 0; i < 2048; i++){
    char buf[32];
    random_bytes(buf, 32);
    dht_node_t *node = dht_node_new(buf);
    dht_bucket_insert(bucket, node);
    if(node == NULL) dht_node_free(node);
  }
  int j = 0;
  dht_bucket_walk(&j, bucket, _walker);
  printf("%i\n", j);
  assert(bucket != NULL);
  dht_bucket_free(bucket);
}

int
main(){
  test_bucket();
  test_bucket_insert();
}