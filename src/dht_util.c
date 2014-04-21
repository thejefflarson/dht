#include "dht_util.h"

int
dht_compare(unsigned char[32], unsigned char[32]){
  for(int i = 0; i < 32; i++){
    uint8_t aint = a->id[i], bint = b->id[i];

    if(aint == bint) continue;

    return aint > bint ? 1 : -1;
  }
  return 0;
}