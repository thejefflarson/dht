#include <stdint.h>
#include "dht_util.h"

void
dht_xor(uint8_t target[32], uint8_t a[32], uint8_t b[32]) {
  for(int i = 0; i < 32; i++) {
    target[i] = a[i] ^ b[i];
  }
}

int
dht_compare(uint8_t a[32], uint8_t b[32]){
  for(int i = 0; i < 32; i++){
    uint8_t aint = a[i], bint = b[i];

    if(aint == bint) continue;

    return aint > bint ? 1 : -1;
  }
  return 0;
}
