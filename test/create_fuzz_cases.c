#include "../dht.c"
#include <sys/stat.h>


int
main(void) {
  request_t req = { .type = 'g', .token = 0, .id = {0} };
  uint8_t buf[sizeof(req) + DHT_HASH_SIZE] = {0};
  memcpy(buf, &req, sizeof(req));
  int fd = open("./fuzz-cases/1.fuzz", O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
  write(fd, buf, sizeof(buf));
  fsync(fd);
  close(fd);
  req.type = 's';
  char data[] = "hello";
  uint8_t sbuf[sizeof(req) + DHT_HASH_SIZE + sizeof(data)] = {0};
  memcpy(sbuf, &req, sizeof(req));
  uint8_t key[DHT_HASH_SIZE] = {0};
  blake2(key, data, NULL, DHT_HASH_SIZE, sizeof(data), 0);
  memcpy(sbuf + sizeof(req), key, DHT_HASH_SIZE);
  memcpy(sbuf + sizeof(req) + DHT_HASH_SIZE, data, sizeof(data));
  fd = open("./fuzz-cases/2.fuzz", O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
  write(fd, sbuf, sizeof(sbuf));
  fsync(fd);
  close(fd);
  return 0;
}
