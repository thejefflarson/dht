#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>
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


#pragma mark types

typedef struct {
  uint8_t id[DHT_HASH_SIZE];
  time_t last_heard;
  struct sockaddr_storage address;
} node_t;

typedef struct bucket_t {
  node_t* nodes[8];
  int length;
  uint8_t max[DHT_HASH_SIZE];
  struct bucket_t *next;
} bucket_t;

#define FILTER_SIZE 128
typedef struct {
  uint64_t token;
  uint8_t key[DHT_HASH_SIZE];
  uint8_t filter[FILTER_SIZE];
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
  dht_store_callback store;
  dht_lookup_callback lookup;
  struct bucket_t *bucket;
};

typedef struct {
  char type;
  uint64_t token;
  uint8_t id[DHT_HASH_SIZE];
} __attribute__((packed)) request_t;

typedef struct {
  char type;
  uint8_t id[DHT_HASH_SIZE];
  uint32_t ip4;
  uint8_t ip6[16];
  uint16_t port;
} __attribute__((packed)) ip_t;

typedef struct {
  uint8_t target[DHT_HASH_SIZE];
  node_t **closest;
  size_t closest_len;
} find_state_t;

typedef int (*bucket_walk_callback)(void *ctx, bucket_t *root);

static int fd = -1;


#pragma mark utils

void
randombytes(uint8_t *x, uint32_t xlen) {
  int i;
  if(fd == -1) {
    for(;;) {
      fd = open("/dev/urandom", O_RDONLY);
      if (fd != -1) break;

      sleep(1);
    }
  }

  while(xlen > 0) {
    if(xlen < 1048576) i = xlen; else i = 1048576;

    i = read(fd,x,i);
    if(i < 1) {
      sleep(1);
      continue;
    }

    x += i;
    xlen -= i;
  }
}


#pragma mark filter

#define hasher(key, i) (*(uint64_t *) (&key[sizeof(uint64_t) * i])) % (CHAR_BIT * FILTER_SIZE)
void
filter_add(uint8_t filter[FILTER_SIZE], uint8_t key[DHT_HASH_SIZE]) {
  for(int i = 0; i < 4; i++) {
    uint64_t hash = hasher(key, i);
    filter[hash / 8] |= (1 << hash % 8);
  }
}

bool
filter_includes(uint8_t filter[FILTER_SIZE], uint8_t key[DHT_HASH_SIZE]) {
  for(int i = 0; i < 4; i++) {
    uint64_t hash = hasher(key, i);
    if((filter[hash / 8] & (1 << (hash % 8))) == 0)
      return false;
  }
  return true;
}


#pragma mark math

static void
xor(uint8_t target[DHT_HASH_SIZE], uint8_t a[DHT_HASH_SIZE], uint8_t b[DHT_HASH_SIZE]) {
  for(int i = 0; i < DHT_HASH_SIZE; i++) {
    target[i] = a[i] ^ b[i];
  }
}

static int
add_ids(const uint8_t a[DHT_HASH_SIZE], const uint8_t b[DHT_HASH_SIZE], uint8_t c[DHT_HASH_SIZE]) {
  uint8_t carry = 0;
  for(int i = DHT_HASH_SIZE - 1; i >= 0; i--){
    uint16_t res = (uint16_t)a[i] + (uint16_t)b[i] + carry;
    carry = res > 0xFF ? 1 : 0;
    c[i] = carry ? res - (0xFF + 1) : res;
  }
  return carry > 0 ? -1 : 0;
}

static int
subtract_ids(const uint8_t a[DHT_HASH_SIZE], const uint8_t b[DHT_HASH_SIZE], uint8_t c[DHT_HASH_SIZE]) {
  int carry = 0;
  for(int i = DHT_HASH_SIZE - 1; i >= 0; i--){
    int res = (int)a[i] - (int)b[i] + carry;
    carry = res < 0 ? -1 : 0;
    c[i] = carry == -1 ? (0xFF + 1) + res : res;
  }
  return carry < 0 ? -1 : 0;
}

static void
divide_by_two(const uint8_t a[DHT_HASH_SIZE], uint8_t b[DHT_HASH_SIZE]) {
  for(int i = DHT_HASH_SIZE - 1; i >= 0; i--) {
    uint8_t it = a[i] >> 1;
    if(i - 1 > 0 && a[i - 1] & 1)
      it |= 0x80;
    b[i] = it;
  }
}


#pragma mark node

static void
node_update(node_t *node){
  time(&node->last_heard);
}

static node_t *
node_new(const uint8_t id[DHT_HASH_SIZE], const struct sockaddr_storage *address) {
  node_t* node = calloc(1, sizeof(node_t));

  if(node == NULL)
    return NULL;

  memcpy(&node->address, address, sizeof(struct sockaddr_storage));
  memcpy(node->id, id, DHT_HASH_SIZE);
  node_update(node);
  return node;
}

static void
node_free(node_t *node){
  free(node);
}

static bool
node_good(node_t *node){
  return time(NULL) - node->last_heard < 60 * 15;
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


#pragma mark bucket

static bool
bucket_contains(bucket_t *root, node_t *node){
  return memcmp(root->max, node->id, DHT_HASH_SIZE) > 0 &&
        (root->next == NULL || memcmp(root->next->max, node->id, DHT_HASH_SIZE) <= 0);
}

static bool
bucket_has_space(bucket_t *root){
  return root->length < 8;
}

static int
bucket_mid(bucket_t* root, uint8_t mid[DHT_HASH_SIZE]){
  int ret;
  if(root->next == NULL) {
    divide_by_two(root->max, mid);
    return 0;
  }
  uint8_t a[DHT_HASH_SIZE] = {0};
  uint8_t b[DHT_HASH_SIZE] = {0};
  ret = subtract_ids(root->max, root->next->max, a);
  if(ret == -1) return -1;
  divide_by_two(a, b);
  ret = add_ids(root->next->max, b, mid);
  if(ret == -1) return -1;
  return 0;
}

static bucket_t*
bucket_new(uint8_t max[DHT_HASH_SIZE]) {
  bucket_t* bucket = calloc(1, sizeof(bucket_t));
  if(bucket == NULL)
    return NULL;
  memcpy(bucket->max, max, DHT_HASH_SIZE);
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
  } else {
    return NULL;
  }

  return root;
}

static void
bucket_walk(void *ctx, bucket_t *root, const bucket_walk_callback cb) {
  while(cb(ctx, root) == 0 && root->next != NULL) {
    root = root->next;
  }
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
    for(size_t j = 0; j < state->closest_len; j++) {
      xor(adelta, state->target, root->nodes[i]->id);
      xor(bdelta, state->target, state->closest[j]->id);

      if(memcmp(adelta, bdelta, DHT_HASH_SIZE) < 0) {
        if(state->closest_len < 8) {
          state->closest_len++;
        }

        memmove(state->closest + j + 1, state->closest + j, sizeof(node_t*) * (state->closest_len - j - 1));
        state->closest[j] = root->nodes[i];
        break;
      }
    }
  }

  return 0;
}

static size_t
find_nodes(node_t *nodes[8], const bucket_t *root, const uint8_t key[DHT_HASH_SIZE]) {
  if(root->length == 0) return 0;

  find_state_t state;
  memset(&state, 0, sizeof(state));
  state.closest = nodes;
  state.closest[0] = root->nodes[0];
  state.closest_len++;
  memcpy(state.target, key, DHT_HASH_SIZE);

  bucket_walk((void *) &state, (bucket_t *)root, find_walker);
  return state.closest_len;
}

static node_t*
find_node(const dht_t *dht, const uint8_t key[DHT_HASH_SIZE]) {
  node_t *nodes[8] = {0};
  size_t ret = find_nodes(nodes, (const bucket_t*) dht->bucket, key);
  if(ret > 0)
    return nodes[0];
  return NULL;
}


#pragma mark dht

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
  hints.ai_protocol = IPPROTO_UDP;
  char cport[6] = {0};
  snprintf(cport, 6, "%i", port);
  int error = getaddrinfo(NULL, cport, &hints, &res);
  if(error != 0) {
    errno = error;
    goto cleanup;
  }

  dht->socket = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
  if(dht->socket == -1) {
    goto cleanup;
  }

  if(bind(dht->socket, res->ai_addr, res->ai_addrlen) != 0) {
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

void
dht_set_storage(dht_t *dht, dht_store_callback store, dht_lookup_callback lookup) {
  dht->store = store;
  dht->lookup = lookup;
}

static search_t *
get_search(dht_t *dht, const uint8_t key[DHT_HASH_SIZE],
           dht_get_callback success, dht_failure_callback error, void *closure) {
  if(dht->search_len + 1 >= MAX_SEARCH) return NULL;
  search_t *to_search = &dht->searches[dht->search_idx[dht->search_len]];
  randombytes((uint8_t*) &to_search->token, sizeof(to_search->token));
  if(key) memcpy(to_search->key, key, DHT_HASH_SIZE);
  to_search->success = success;
  to_search->error = error;
  time(&to_search->sent);
  to_search->data = closure;
  dht->search_len++;
  return to_search;
}

static ssize_t
compress_and_send(const dht_t *dht, const node_t *node,
                  const uint8_t *buf, const size_t len) {
  size_t length = compressBound(len);
  uint8_t *comp = (uint8_t *) calloc(1, length);
  if(!comp) return -1;
  ssize_t ret = compress(comp, &length, buf, len);
  if(ret != Z_OK) {
    free(comp);
    return -1;
  }

  ret = sendto(dht->socket, comp, length, 0, (struct sockaddr *) &node->address, node->address.ss_len);
  free(comp);
  return ret;
};

static int
send_get(dht_t *dht, search_t *to_search, node_t *node) {
  request_t get = { .type = 'g' };
  get.token = to_search->token;
  memcpy(get.id, dht->id, DHT_HASH_SIZE);
  uint8_t buf[sizeof(get) + DHT_HASH_SIZE] = {0};
  memcpy(buf, &get, sizeof(get));
  memcpy(buf + sizeof(get), to_search->key, DHT_HASH_SIZE);
  filter_add(to_search->filter, node->id);
  return compress_and_send(dht, node, (uint8_t *)&buf, sizeof(buf));
}

int
dht_add_node(dht_t *dht, uint8_t key[], struct sockaddr_storage *addr) {
  node_t *node = node_new(key, addr);
  if(!node) return -1;
  bucket_t *ret = bucket_insert(dht->bucket, node);
  if(!ret) {
    node_free(node);
    return -1;
  }
  return 0;
}

int
dht_get(dht_t *dht, uint8_t key[DHT_HASH_SIZE],
        dht_get_callback success, dht_failure_callback error, void *closure) {
  node_t *node = find_node(dht, key);
  if(!node) return -1;
  search_t *to_search = get_search(dht, key, success, error, closure);
  if(!to_search) return -1;
  return send_get(dht, to_search, node);
}

int
dht_set(dht_t *dht, void *data, size_t len, dht_get_callback success,
        dht_failure_callback error, void *closure) {
  uint8_t key[DHT_HASH_SIZE] = {0};
  int ret = blake2(key, data, NULL, DHT_HASH_SIZE, len, 0);
  if(ret == -1) return ret;
  node_t *node = find_node(dht, key);
  if(!node) return -1;
  search_t *to_search = get_search(dht, key, success, error, closure);
  if(!to_search) return -1;

  request_t set = { .type = 's' };
  set.token = to_search->token;
  memcpy(set.id, dht->id, DHT_HASH_SIZE);

  uint8_t *buf = calloc(1, sizeof(set) + DHT_HASH_SIZE + len);
  if(buf == NULL) return -1;
  memcpy(buf, &set, sizeof(set));
  memcpy(buf + sizeof(set), key, DHT_HASH_SIZE);
  memcpy(buf + sizeof(set) + DHT_HASH_SIZE, data, len);
  ret = compress_and_send(dht, node, buf, sizeof(set) + len + DHT_HASH_SIZE);
  free(buf);
  return ret;
}

static ssize_t
search_idx(dht_t *dht, uint64_t token) {
  for(int i = 0; i < dht->search_len; i++) {
    if(dht->searches[dht->search_idx[i]].token == token){
      return i;
    }
  }
  return -1;
}

static search_t *
find_search(dht_t *dht, uint64_t token) {
  ssize_t idx = search_idx(dht, token);
  return idx == -1 ? NULL : &dht->searches[idx];
}

static void
kill_search(dht_t *dht, int idx) {
  uint16_t tmp = dht->search_idx[dht->search_len - 1];
  dht->search_idx[dht->search_len - 1] = dht->search_idx[idx];
  dht->search_idx[idx] = tmp;
  dht->search_len--;
}

static void
fill_ip(ip_t *ip, const node_t *node) {
  memcpy(ip->id, node->id, DHT_HASH_SIZE);
  ip->type = node->address.ss_family == AF_INET ? '4' : '6';
  switch(ip->type) {
    case('4'): {
      ip->ip4 = htonl(((struct sockaddr_in *) &node->address)->sin_addr.s_addr);
      break;
    }
    case('6'): {
      uint8_t *addr = ((struct sockaddr_in6 *) &node->address)->sin6_addr.s6_addr;
      memcpy(ip->ip6, addr, 16);
      break;
    }
  }
  ip->port = htonl(((struct sockaddr_in *) &node->address)->sin_port);
}

static void
insert_from_ip(dht_t *dht, const ip_t *ip) {
  struct sockaddr_storage address = {0};
  switch(ip->type) {
    case('4'):
      ((struct sockaddr_in *) &address)->sin_addr.s_addr = ntohl(ip->ip4);
      break;
    case('6'): {
      uint8_t *addr = ((struct sockaddr_in6 *) &address)->sin6_addr.s6_addr;
      memcpy(addr, ip->ip6, 16);
      break;
    }
    default:
      return;
  }
  node_t *node = node_new(ip->id, &address);
  bucket_insert(dht->bucket, node);
}

#define MAX_SIZE 1500

static ssize_t
create_get_response(dht_t* dht,
           const uint64_t token,
           const uint8_t key[DHT_HASH_SIZE],
           uint8_t **buf) {
  request_t resp = { .type = 'h' };
  memcpy(resp.id, dht->id, DHT_HASH_SIZE);
  resp.token = token;
  if(dht->lookup) {
    void *value = NULL;
    ssize_t ret = dht->lookup(key, &value);
    if(ret > 0) {
      *buf = calloc(1, sizeof(resp) + ret);
      if(!*buf) return -1;
      memcpy(*buf, (void *)&resp, sizeof(resp));
      memcpy(*buf + sizeof(resp), value, ret);
      return sizeof(resp) + ret;
    }
  }
  resp.type = 'i';
  node_t *nodes[8];
  size_t found = find_nodes(nodes, dht->bucket, key);
  *buf = calloc(1, sizeof(resp) + found * sizeof(ip_t));
  if(!*buf) return -1;
  memcpy(*buf, &resp, sizeof(resp));
  for(size_t i = 0; i < found; i++) {
    ip_t ip = {0};
    fill_ip(&ip, nodes[i]);
    memcpy(*buf + sizeof(resp) + sizeof(ip_t) * i, &ip, sizeof(ip));
  }
  return sizeof(resp) + sizeof(ip_t) * found;
}

typedef struct {
  node_t *node;
  bucket_t *bucket;
} ping_t;

static void
remove_node(void *ctx){
  ping_t *ping = ctx;
  bucket_t *root = ping->bucket;
  for(int i = 0; i < root->length; i++){
    if(crypto_verify_32(ping->node->id, root->nodes[i]->id) == 0){
      node_free(root->nodes[i]);
      memmove(root->nodes + i, root->nodes + i + 1, sizeof(node_t *) * (root->length - i - 1));
      i--;
      root->length--;
    }
  }
  qsort(root->nodes, root->length, sizeof(node_t *), node_sort);
  free(ping);
}


static int
bucket_ping_walker(void *ctx, bucket_t *root) {
  dht_t *dht = ctx;
  for(int i = 0; i < root->length; i++) {
    node_t *node = root->nodes[i];
    if(!node_good(node)) {
      ping_t *ping = calloc(1, sizeof(ping_t));
      if(ping == NULL) continue;
      search_t *search = get_search(dht, node->id, NULL, remove_node, root);
      if(search == NULL) {free(ping); continue; }
      request_t req = { .type = 'p' };
      memcpy(req.id, dht->id, DHT_HASH_SIZE);
      req.token = search->token;
      compress_and_send(dht, ping->node, (uint8_t *)&req, sizeof(req));
    }
  }
  return 0;
}

static void
bucket_ping(dht_t *dht) {
  bucket_walk(dht, dht->bucket, bucket_ping_walker);
}

int
dht_run(dht_t *dht, int timeout) {
  // clear old searches
  for(int i = 0; i < dht->search_len; i++) {
    search_t search = dht->searches[dht->search_idx[i]];
    if((time(NULL) - search.sent) > 60) {
      kill_search(dht, i);
      search.error(search.data);
    }
  }

  node_t *node = NULL;
  struct pollfd fd = {0};
  fd.fd = dht->socket;
  fd.events = POLLIN;
  int ev = poll(&fd, 1, timeout);

  if(ev <= 0) return -1;
  if(!(fd.revents & POLLIN)) return 0;

  uint8_t buf[MAX_SIZE] = {0};
  struct sockaddr_storage addr = {0};
  socklen_t len = sizeof(addr);
  ssize_t ret = recvfrom(dht->socket, buf, MAX_SIZE, 0, (struct sockaddr *)&addr, &len);
  if(ret == -1) return ret;

  uint8_t big[MAX_SIZE] = {0};
  size_t big_len = MAX_SIZE;
  ret = uncompress(big, &big_len, buf, ret);
  if(ret != Z_OK)
    goto cleanup;

  if(big_len < sizeof(request_t))
    goto cleanup;

  request_t *request = (request_t *)big;
  if(request->type == 'o' ||
     request->type == 'h' ||
     request->type == 'i' ||
     request->type == 't') {
    // we don't recognize this search, bail
    if(search_idx(dht, request->token) == -1) goto cleanup;
  }

  if(request->type == 'g' ||
     request->type == 's') {
    // request too small
    if(big_len < sizeof(request_t) + DHT_HASH_SIZE) goto cleanup;

    if(request->type == 's')
      if(big_len - sizeof(request_t) == 0) goto cleanup;
  }

  node = find_node(dht, request->id);
  if(node == NULL || memcmp(node->id, request->id, DHT_HASH_SIZE) != 0) {
    node = node_new(request->id, &addr);
    if(node == NULL) goto cleanup;
    bucket_insert(dht->bucket, node);
  }

  //if(memcmp(&node->address, &addr, sizeof(addr)) != 0) return -1; // TOFU

  node_update(node);

  switch(request->type) {
    case 'p': { // ping
      request_t resp = { .type = 'o' };
      memcpy(resp.id, dht->id, DHT_HASH_SIZE);
      resp.token = request->token;
      compress_and_send(dht, node, (uint8_t *)&resp, sizeof(resp));
      break;
    }
    case 'g': { // get
      uint8_t key[DHT_HASH_SIZE] = {0};
      memcpy(key, big + sizeof(request_t), DHT_HASH_SIZE);
      uint8_t *buf = NULL;
      ssize_t ret = create_get_response(dht, request->token, key, &buf);
      if(ret != -1) {
        compress_and_send(dht, node, buf, ret);
        free(buf);
      } else {
        goto cleanup;
      }
      break;
    }
    case 'h':{ // get response found
      search_t *search = find_search(dht, request->token);
      uint8_t key[DHT_HASH_SIZE];
      uint8_t *data = big + sizeof(request_t);
      size_t len = big_len - sizeof(request_t);
      int ret = blake2(key, data, NULL, DHT_HASH_SIZE, len, 0);
      if(ret != -1 && crypto_verify_32(key, search->key) == 0) {
        search->success(search->data, search->key, data, len);
        kill_search(dht, search_idx(dht, search->token));
      }
      break;
    }
    case 'i': { // get response not found
      uint8_t *data = big + sizeof(request_t);
      for(size_t i = 0; i < (big_len - sizeof(request_t)) / sizeof(ip_t); i++) {
        ip_t *ip = (ip_t *)data + sizeof(request_t) * i;
        insert_from_ip(dht, ip);
      }
      search_t *search = find_search(dht, request->token);
      node_t *nodes[8];
      size_t found = find_nodes(nodes, dht->bucket, search->key);
      for(size_t i = 0; i < found; i++)
        if(!filter_includes(search->filter, node->id))
          send_get(dht, search, nodes[i]);
      break;
    }
    case 's': { //
      uint8_t hash[DHT_HASH_SIZE] = {0};
      uint8_t *data =  big + sizeof(request_t) + DHT_HASH_SIZE;
      int ret = blake2(hash, data, NULL, DHT_HASH_SIZE, big_len - sizeof(request_t) - DHT_HASH_SIZE, 0);
      if(ret != -1 && crypto_verify_32(hash, big + sizeof(request_t)) == 0) {
        if(dht->store) {
          dht->store(hash, data, big_len - sizeof(request_t) - DHT_HASH_SIZE);
        }
        request_t resp = {0};
        resp.type = 't';
        memcpy(resp.id, dht->id, DHT_HASH_SIZE);
        resp.token = request->token;
        compress_and_send(dht, node, (uint8_t *)&resp, sizeof(resp));
      }
      break;
    }
    case 'o':
    case 't': // set response
      kill_search(dht, search_idx(dht, request->token));
      break;
    default:
      break;
  }

  bucket_ping(dht);
  return 0;
cleanup:
  return -1;
}
