#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <time.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <zlib.h>

#include "vendor/blake2.h"
#include "vendor/tweetnacl.h"

#include "dht.h"

typedef struct dh_node_t {
  uint8_t id[DHT_HASH_SIZE];
  time_t created_at;
  time_t last_heard;
  struct sockaddr_storage address;
} node_t;

typedef struct bucket_t {
  node_t* nodes[8];
  int length;
  uint8_t max[DHT_HASH_SIZE];
  struct bucket_t *next;
} bucket_t;

typedef struct {
  uint8_t token[DHT_HASH_SIZE];
  uint8_t key[DHT_HASH_SIZE];
  time_t sent;
  void* data;
  dht_get_callback success;
  dht_failure_callback error;
} search_t;

#define MAX_SEARCH 1024
struct dht_s {
  int socket;
  uint8_t id[DHT_HASH_SIZE];
  search_t searches[MAX_SEARCH];
  uint16_t search_idx[MAX_SEARCH];
  uint16_t search_len;
  struct bucket_t *bucket;
};

typedef struct {
  char type;
  uint8_t token[DHT_HASH_SIZE];
  uint8_t id[DHT_HASH_SIZE];
} __attribute__((packed)) request_t;

typedef struct {
  uint8_t target[DHT_HASH_SIZE];
  node_t *current;
} find_state_t;

typedef int (*bucket_walk_callback)(void *ctx, bucket_t *root);


static int fd = -1;

void
randombytes(unsigned char *x,unsigned long long xlen) {
  int i;

  if (fd == -1) {
    for (;;) {
      fd = open("/dev/urandom",O_RDONLY);
      if (fd != -1) break;
      sleep(1);
    }
  }

  while (xlen > 0) {
    if (xlen < 1048576) i = xlen; else i = 1048576;

    i = read(fd,x,i);
    if (i < 1) {
      sleep(1);
      continue;
    }

    x += i;
    xlen -= i;
  }
}

static void
xor(uint8_t target[DHT_HASH_SIZE], uint8_t a[DHT_HASH_SIZE], uint8_t b[DHT_HASH_SIZE]) {
  for(int i = 0; i < DHT_HASH_SIZE; i++) {
    target[i] = a[i] ^ b[i];
  }
}

static int
compare(uint8_t a[DHT_HASH_SIZE], uint8_t b[DHT_HASH_SIZE]){
  for(int i = 0; i < DHT_HASH_SIZE; i++){
    uint8_t aint = a[i], bint = b[i];

    if(aint == bint) continue;

    return aint > bint ? 1 : -1;
  }
  return 0;
}

static void
node_update(node_t *node){
  time(&node->last_heard);
}

static node_t *
node_new(const uint8_t id[DHT_HASH_SIZE], const struct sockaddr_storage *address) {
  node_t* node = calloc(1, sizeof(node_t));

  if(node == NULL)
    return NULL;

  memcpy((void *) &node->address, (const void *) address, sizeof(struct sockaddr_storage));
  memcpy(node->id, id, DHT_HASH_SIZE);
  time(&node->created_at);
  node_update(node);
  return node;
}

static void
node_free(node_t *node){
  free(node);
}

static bool
node_good(node_t *node){
  return time(NULL) - node->last_heard < 1500;
}

static int
node_sort(const void* a, const void *b) {
  node_t *aa = * (node_t * const *) a;
  node_t *bb = * (node_t * const *) b;
  if(aa->last_heard > bb->last_heard) {
    return 1;
  } else if (aa->last_heard < bb->last_heard) {
    return -1;
  } else {
    return 0;
  }
}

// we treat node ids as big endian integers so we can use a simple memcmp below
// neat tweak from jech/dht.c
static bool
bucket_contains(bucket_t *root, node_t *node){
  return memcmp(root->max, node->id, DHT_HASH_SIZE) < 0 && 
        (root->next == NULL || memcmp(root->next->max, node->id, DHT_HASH_SIZE));
}

static bool
bucket_has_space(bucket_t *root){
  return root->length < 8;
}

static int
add_ids(const uint8_t a[DHT_HASH_SIZE], const uint8_t b[DHT_HASH_SIZE], uint8_t c[DHT_HASH_SIZE]) {
  uint8_t carry = 0;
  for(int i = DHT_HASH_SIZE - 1; i >= 0; i--){
    uint16_t res = (uint16_t)a[i] + (uint16_t)b[i] + carry;
    carry = res > 0xFF ? res - 0xFF : 0;
    c[i] = res > 0xFF ? 0xFF : res;
  }
  return carry > 0 ? -1 : 0;
}

static int
subtract_ids(const uint8_t a[DHT_HASH_SIZE], const uint8_t b[DHT_HASH_SIZE], uint8_t c[DHT_HASH_SIZE]) {
  int carry = 0;
  for(int i = DHT_HASH_SIZE - 1; i >= 0; i--){
    int res = (int)a[i] - (int)b[i] + carry;
    carry = res;
    c[i] = res < 0 ? 0 : res;
  }
  return carry < 0 ? -1 : 0;
}

static int
divide_by_two(const a[DHT_HASH_SIZE], uint8_t b[DHT_HASH_SIZE]) {

}

static int
bucket_mid(bucket_t* root, uint8_t mid[DHT_HASH_SIZE]){
    
  return (root->upper_limit - root->lower_limit) / 2 + root->lower_limit;
}

static bucket_t*
bucket_new(uint8_t max[DHT_HASH_SIZE]) {
  bucket_t* bucket = calloc(1, sizeof(bucket_t));
  if(bucket == NULL)
    return NULL;
  memcpy(bucket, max, DHT_HASH_SIZE);
  bucket->next = NULL;
  return bucket;
}

static bucket_t*
bucket_insert(bucket_t *root, node_t *node);

static bool
bucket_split(bucket_t *root){
  uint8_t mid[DHT_HASH_SIZE] = {0};
  bool res = bucket_mid(root, mid);
  if(res) return true;
  bucket_t *next = bucket_new(mid);
  if(next == NULL)
    return true;

  next->next = root->next;
  root->next = next;

  for(int i = 0; i < root->length; i++){
    if(!bucket_contains(root, root->nodes[i])) {
      if(bucket_insert(next, root->nodes[i])) {
        if(i + 1 < root->length)
          memmove(root->nodes + i, root->nodes + i + 1, sizeof(node_t *) * (root->length - i - 1));
        i--;
        root->length--;
      }
    }
  }

  return false;
}

static void
bucket_update(bucket_t *root);

static bucket_t*
bucket_insert(bucket_t *root, node_t *node) {
  while(root != NULL && !bucket_contains(root, node))
    root = root->next;

  if(root == NULL){
    return NULL; // shouldn't happen
  }

  if(!bucket_has_space(root)) {
    bool err = bucket_split(root);
    if(err) return NULL;
  }

  // have to check again to see if some nodes moved over
  if(bucket_has_space(root)) {
    root->nodes[root->length++] = node;
    bucket_update(root);
  } else {
    return NULL;
  }

  return root;
}

static void
bucket_walk(void *ctx, bucket_t *root, bucket_walk_callback cb) {
  while(cb(ctx, root) == 0 && root->next != NULL) {
    root = root->next;
  }
}

static int
bucket_update_walker(void *ctx, bucket_t *root){
  (void) ctx;
  for(int i = 0; i < root->length; i++){
    if(!node_good(root->nodes[i])){
      node_free(root->nodes[i]);
      memmove(root->nodes + i, root->nodes + i + 1, sizeof(node_t *) * (root->length - i - 1));
      i--;
      root->length--;
    }
  }
  qsort(root->nodes, root->length, sizeof(node_t *), node_sort);
  return 0;
}

static void
bucket_update(bucket_t *root) {
  bucket_walk(NULL, root, bucket_update_walker);
}

static void
bucket_free(bucket_t *root) {
  bucket_t *b;
  while(root != NULL) {
    for(int i = 0; i < root->length; i++)
      node_free(root->nodes[i]);
    b = root;
    root = b->next;
    free(b);
  }
}

static int
find_walker(void *ctx, bucket_t *root){
  find_state_t *state = ctx;
  uint8_t adelta[DHT_HASH_SIZE], bdelta[DHT_HASH_SIZE];

  for(int i = 0; i < root->length; i++){
    xor(adelta, state->target, root->nodes[i]->id);
    xor(bdelta, state->target, state->current->id);

    if(compare(adelta, bdelta) == -1)
      state->current = root->nodes[i];
  }

  return 0;
}

static node_t *
find_node(dht_t *dht, uint8_t key[DHT_HASH_SIZE]) {
  if(dht->bucket->length == 0) return NULL;

  find_state_t state;

  state.current = dht->bucket->nodes[0];
  memcpy(state.target, key, DHT_HASH_SIZE);

  bucket_walk((void *) &state, dht->bucket, find_walker);

  return state.current;
}

dht_t *
dht_new(int port) {
  dht_t *dht = calloc(1, sizeof(dht_t));
  if(!dht) return NULL;
  uint8_t key[DHT_HASH_SIZE] = {0xFF};
  dht->bucket = bucket_new(key);
  if(dht->bucket == NULL) {
    goto error;
  }

  randombytes(dht->id, DHT_HASH_SIZE);

  struct addrinfo hints = {0}, *res = NULL;
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_DGRAM;
  hints.ai_flags = AI_PASSIVE;
  char cport[6] = {0};
  snprintf(cport, 6, "%i", port);
  int error = getaddrinfo(NULL, cport, &hints, &res);
  if(error) {
    errno = error;
    goto cleanup;
  }

  dht->socket = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
  if(dht->socket == -1) {
    goto cleanup;
  }

  if(bind(dht->socket, res->ai_addr, res->ai_addrlen) == -1) {
    goto cleanup;
  }
  freeaddrinfo(res);
  for(int i = 0; i < MAX_SEARCH; i++)
    dht->search_idx[i] = i;

  return dht;
cleanup:
  if(res != NULL) freeaddrinfo(res);
  bucket_free(dht->bucket);
error:
  free(dht);
  return NULL;
}

void
dht_close(dht_t *dht) {
  bucket_free(dht->bucket);
  close(dht->socket);
  free(dht);
}

search_t *
get_search(dht_t *dht, const uint8_t key[DHT_HASH_SIZE], dht_get_callback success, dht_failure_callback error, void *closure) {
  if(dht->search_len + 1 >= MAX_SEARCH) return NULL;
  search_t *to_search = &dht->searches[dht->search_idx[dht->search_len]];
  randombytes(to_search->token, DHT_HASH_SIZE);
  if(key) memcpy(to_search->key, key, DHT_HASH_SIZE);
  to_search->success = success;
  to_search->error = error;
  time(&to_search->sent);
  to_search->data = closure;
  dht->search_len++;
  return to_search;
}

int
compress_and_send(const dht_t *dht, const node_t *node, const uint8_t *buf, const size_t len) {
  uint8_t *comp = (uint8_t *) calloc(1, len);
  if(!comp) return -1;
  size_t length;
  size_t ret = compress(comp, &length, buf, len);
  if(ret != Z_OK) {
    free(comp);
    return -1;
  }

  ret = sendto(dht->socket, comp, length, 0, (struct sockaddr *)&node->address, sizeof(node->address));
  free(comp);
  return ret;
};

int
dht_get(dht_t *dht, uint8_t key[DHT_HASH_SIZE], dht_get_callback success, dht_failure_callback error, void *closure) {
  node_t *node = find_node(dht, key);
  if(!node) return -1;
  search_t *to_search = get_search(dht, key, success, error, closure);
  if(!to_search) return -1;

  request_t get = { .type = 'g' };
  memcpy(get.token, to_search->token, DHT_HASH_SIZE);
  memcpy(get.id, dht->id, DHT_HASH_SIZE);
  uint8_t buf[sizeof(get) + DHT_HASH_SIZE] = {0};
  memcpy(buf, &get, sizeof(get));
  memcpy(buf + sizeof(get), key, DHT_HASH_SIZE);

  return compress_and_send(dht, node, (uint8_t *)&buf, sizeof(buf));
}

int
dht_set(dht_t *dht, void *data, size_t len, dht_get_callback success, dht_failure_callback error, void *closure) {
  uint8_t key[DHT_HASH_SIZE] = {0};
  int ret = blake2(key, data, NULL, DHT_HASH_SIZE, len, 0);
  if(ret == -1) return ret;
  node_t *node = find_node(dht, key);
  if(!node) return -1;
  search_t *to_search = get_search(dht, key, success, error, closure);
  if(!to_search) return -1;

  request_t set = { .type = 's' };
  memcpy(set.token, to_search->token, DHT_HASH_SIZE);
  memcpy(set.id, dht->id, DHT_HASH_SIZE);

  uint8_t *buf = calloc(1, sizeof(set) + DHT_HASH_SIZE + len);
  if(buf == NULL) return -1;
  memcpy(buf, &set, sizeof(set));
  memcpy(buf + sizeof(set), key, DHT_HASH_SIZE);
  memcpy(buf + sizeof(set) + DHT_HASH_SIZE, data, len);
  ret = compress_and_send(dht, node, buf, sizeof(set) + len);
  free(buf);
  return ret;
}

static void
kill_search(dht_t *dht, int idx) {
  search_t search = dht->searches[dht->search_idx[idx]];
  uint16_t tmp = dht->search_idx[dht->search_len - 1];
  dht->search_idx[dht->search_len - 1] = dht->search_idx[idx];
  dht->search_idx[idx] = tmp;
  dht->search_len--;
  search.error(search.data);
}


#define MAX_SIZE 1500

int
dht_run(dht_t *dht, int timeout) {
  // clear old searches
  for(int i = 0; i < dht->search_len; i++) {
    search_t search = dht->searches[dht->search_idx[i]];
    if(time(NULL) - search.sent > 60) {
      kill_search(dht, i);
    }
  }

  node_t *node = NULL;
  struct pollfd fd = {0};
  fd.fd = dht->socket;
  fd.events = POLLIN;
  poll(&fd, 1, timeout);

  if(!(fd.revents & POLLIN)) return 0;

  uint8_t buf[MAX_SIZE] = {0};
  struct sockaddr_storage addr = {0};
  socklen_t len;
  size_t ret = recvfrom(dht->socket, buf, MAX_SIZE, 0, (struct sockaddr *)&addr, &len);
  if(ret == -1) return ret;

  uint8_t *big = calloc(1, ret);
  if(big == NULL) return -1;
  size_t big_len;
  ret = uncompress(big, &big_len, buf, ret);
  if(ret != Z_OK)
    goto cleanup;

  request_t *request = (request_t *)big;

  if(request->type == 'o' ||
     request->type == 'h' ||
     request->type == 'i' ||
     request->type == 't') {
    bool killed = false;
    for(int i = 0; dht->search_len; i++) {
      if(crypto_verify_32(dht->searches[dht->search_idx[i]].token, request->token)){
        kill_search(dht, i);
        killed = true;
        break;
      }
    }
    // we don't recognize this search, bail
    if(!killed) goto cleanup;
  }

  node = find_node(dht, request->id);
  if(node == NULL || compare(node->id, request->id) != 0) {
    node = node_new(request->id, &addr);
    if(node == NULL) goto cleanup;
    bucket_insert(dht->bucket, node);
  }

  if(!memcmp(&node->address, &addr, sizeof(addr))) return -1; // TOFU

  node_update(node);

  switch(request->type) {
    case 'p': { // ping
      request_t resp = { .type = 'o' };
      memcpy(resp.id, dht->id, DHT_HASH_SIZE);
      memcpy(resp.token, request->token, DHT_HASH_SIZE);
      compress_and_send(dht, node, (uint8_t *)&resp, sizeof(resp));
      break;
    }
    case 'o':
      break;

    case 'g': { // get

      break;
    }
    case 'h': // get response found
      break;
    case 'i': // get response not found
      break;
    case 's': // set
      break;
    case 't': // set response
      break;
  }

  bucket_update(dht->bucket);

  return 0;
cleanup:
  free(big);
  if(node) free(node);
  return -1;
}
