#include <stdint.h>
#include "dht_util.h"

void
dht_xor(unsigned char target[32], unsigned char a[32], unsigned char b[32]) {
  for(int i = 0; i < 32; i++) {
    target[i] = a[i] ^ b[i];
  }
}


int
dht_compare(unsigned char a[32], unsigned char b[32]){
  for(int i = 0; i < 32; i++){
    uint8_t aint = a[i], bint = b[i];

    if(aint == bint) continue;

    return aint > bint ? 1 : -1;
  }
  return 0;
}