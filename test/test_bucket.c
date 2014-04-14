#include <assert.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
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

void
test_bucket_insert(){

}

int
main(){
  test_bucket();
}