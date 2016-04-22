#include "../dht.c"

int
main(void) {
  uint8_t buf[1500] = {0};
  ssize_t len = fread(buf, 1, sizeof(buf), stdin);
  dht_t *dht = dht_new(10000);
  struct sockaddr_storage addr = {0};
  socklen_t slen = sizeof(addr);
  getsockname(dht->socket, (struct sockaddr *)&addr, &slen);
  uint8_t key[DHT_HASH_SIZE] = {0};
  node_t* node = node_new(key, &addr);
  compress_and_send(dht, node, buf, len);
  dht_run(dht, 1);
  dht_close(dht);
  node_free(node);
  return 0;
}
