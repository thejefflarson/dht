#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>

void
random_bytes(uint8_t *buf, size_t size){
  int f = open("/dev/urandom", O_RDONLY);
  if(!f) exit(1);
  read(f, buf, size);
  close(f);
}
