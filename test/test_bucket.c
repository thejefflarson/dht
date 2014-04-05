#include <assert.h>
#include "dht_bucket.h"

void
test_bucket(){
  dht_bucket_t *bucket = dht_bucket_new(0, 255);
  assert(bucket != NULL);
  dht_bucket_free(bucket);
}

int
main(int argc, char **argv){
  test_bucket();
}