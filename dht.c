#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <time.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "vendor/cmp.h"

#include "dht.h"

static void
random_bytes(uint8_t *buf, size_t size){
  int f = open("/dev/urandom", O_RDONLY);
  if(!f) exit(1);
  read(f, buf, size);
  close(f);
}


static void
xor(uint8_t target[32], uint8_t a[32], uint8_t b[32]) {
  for(int i = 0; i < 32; i++) {
    target[i] = a[i] ^ b[i];
  }
}

static int
compare(uint8_t a[32], uint8_t b[32]){
  for(int i = 0; i < 32; i++){
    uint8_t aint = a[i], bint = b[i];

    if(aint == bint) continue;

    return aint > bint ? 1 : -1;
  }
  return 0;
}

typedef struct dh_node_t {
  uint8_t id[32];
  time_t created_at;
  time_t last_heard;
  struct sockaddr_storage address;
} node_t;

static void
node_update(node_t *node){
  time(&node->last_heard);
}

static node_t *
node_new(const uint8_t id[32], const struct sockaddr_storage *address) {
  node_t* node = calloc(1, sizeof(node_t));

  if(node == NULL)
    return NULL;

  memcpy((void *) &node->address, (const void *) address, sizeof(struct sockaddr_storage));
  memcpy(node->id, id, 32);
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

typedef struct bucket_t {
  node_t* nodes[8];
  int length;
  uint8_t upper_limit;
  uint8_t lower_limit;
  struct bucket_t *next;
} bucket_t;

static bool
bucket_contains(bucket_t *root, node_t *node){
  return root->upper_limit > (uint8_t) node->id[0] && root->lower_limit <= (uint8_t) node->id[0];
}

static bool
bucket_has_space(bucket_t *root){
  return root->length < 8;
}

static uint8_t
bucket_mid(bucket_t* root){
  return (root->upper_limit - root->lower_limit) / 2 + root->lower_limit;
}

static bucket_t*
bucket_new(uint8_t lower, uint8_t upper) {
  bucket_t* bucket = calloc(1, sizeof(bucket_t));
  if(bucket == NULL)
    return NULL;
  bucket->upper_limit = upper;
  bucket->lower_limit = lower;
  bucket->next = NULL;
  return bucket;
}

static bucket_t*
bucket_insert(bucket_t *root, node_t *node);

static bool
bucket_split(bucket_t *root){
  if((root->upper_limit - root->lower_limit) == 1)
    return true; // can't split anymore

  uint8_t mid = bucket_mid(root);

  bucket_t *next = bucket_new(mid, root->upper_limit);
  if(next == NULL)
    return true;

  next->next = root->next;
  root->next = next;
  root->upper_limit = mid;

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

typedef int (*bucket_walk_callback)(void *ctx, bucket_t *root);
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

struct _find_state {
  uint8_t target[32];
  node_t *current;
};

static int
find_walker(void *ctx, bucket_t *root){
  struct _find_state *state = ctx;
  uint8_t adelta[32], bdelta[32];

  for(int i = 0; i < root->length; i++){
    xor(adelta, state->target, root->nodes[i]->id);
    xor(bdelta, state->target, state->current->id);

    if(compare(adelta, bdelta) == -1)
      state->current = root->nodes[i];
  }

  return 0;
}

typedef struct {
  uint8_t *buf;
  size_t size;
  size_t offset;
} string_t;

static size_t
write_buf(cmp_ctx_t *ctx, const void *data, size_t count) {
  string_t *string = ctx->buf;
  if(count + string->offset < string->size) {
    memcpy(string->buf + string->offset, data, count);
    string->offset += count;
    return count;
  }
  return -1;
}


typedef struct {
  uint8_t token[DHT_HASH_SIZE];
  time_t sent;
  void* data;
  dht_get_callback success;
  dht_failure_callback error;
} search;


#define MAX_SEARCH 1024
// all that for these:
struct dht_s {
  int socket;
  search searches[MAX_SEARCH];
  uint16_t search_len;
  struct bucket_t *bucket;
};

static node_t *
find_node(dht_t *dht, uint8_t key[DHT_HASH_SIZE]) {
  if(dht->bucket->length == 0) return NULL;

  struct _find_state state;

  state.current = dht->bucket->nodes[0];
  memcpy(state.target, key, DHT_HASH_SIZE);

  bucket_walk((void *) &state, dht->bucket, find_walker);

  return state.current;
}

int
dht_init(dht_t *dht, int port){
  dht->bucket = bucket_new(0, 255);
  if(dht->bucket == NULL) {
    goto error;
  }

  struct addrinfo hints = {0}, *res;
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_DGRAM;
  hints.ai_flags = AI_PASSIVE;
  char cport[6] = {0};
  snprintf(cport, 6, "%i", port);
  int error = getaddrinfo(NULL, cport, &hints, &res);
  if(error) {
    errno = error;
    bucket_free(dht->bucket);
    return -2;
  }

  dht->socket = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
  if(dht->socket == -1) {
    goto cleanup;
  }

  if(bind(dht->socket, res->ai_addr, res->ai_addrlen) == -1) {
    goto cleanup;
  }

  return 0;
cleanup:
  bucket_free(dht->bucket);
error:
  return -1;
}

void
dht_close(dht_t *dht) {
  bucket_free(dht->bucket);
  close(dht->socket);
}

int
dht_get(dht_t *dht, uint8_t key[32], dht_get_callback success, dht_failure_callback error, void *closure) {
  node_t *node = find_node(dht, key);
  if(!node) return -1;
  search *to_search = &dht->searches[dht->search_len];
  random_bytes(to_search->token, DHT_HASH_SIZE);
  to_search->success = success;
  to_search->error = error;
  time(&to_search->sent);
  to_search->data = closure;
  dht->search_len++;




  return send(dht->socket, data, length, node->address);
}

int
dht_run(dht_t *dht) {
  return -1;
  // switch(op){
  //   case DHT_PING:
  //     node_t* node = dht_find_node(dht, node_id);
  //     if(node == NULL) return -1;
  //     node_update(node);
  //     dht->send(dht, DHT_PONG, NULL, 0, node);
  //     return 1;
  //   case DHT_FIND_NODES:
  //     // todo: implement nearest_nodes and send
  //     node_list_t* nodes = dht_find_nearest_nodes(dht, node_id, 10);
  //     if(nodes == NULL) return -1;
  //     dht->send(dht, DHT_NODES, node, sizeof(node), node);
  //     return nodes->length;
  //   case DHT_ANNOUNCE:
  //     // add to dht
  //     return 1;
  //   default:
  //     return -1;
  // }
}
