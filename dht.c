#include <netinet/in.h>
#include <time.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "dht.h"

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


void
node_update(node_t *node){
  time(&node->last_heard);
}

node_t *
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

void
node_free(node_t *node){
  free(node);
}

bool
node_good(node_t *node){
  return time(NULL) - node->last_heard < 1500;
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

bucket_t*
bucket_new(uint8_t lower, uint8_t upper) {
  bucket_t* bucket = calloc(1, sizeof(bucket_t));
  if(bucket == NULL)
    return NULL;
  bucket->upper_limit = upper;
  bucket->lower_limit = lower;
  bucket->next = NULL;
  return bucket;
}

bucket_t*
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

void
bucket_update(bucket_t *root);

bucket_t*
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

// TODO: make non-recursive
typedef int (*bucket_walk_callback)(void *ctx, bucket_t *root);
void
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
      if(i + 1 < root->length)
        memmove(root->nodes + i, root->nodes + i + 1, sizeof(node_t *) * (root->length - i - 1));
      i--;
      root->length--;
    }
  }
  return 0;
}

void
bucket_update(bucket_t *root) {
  bucket_walk(NULL, root, bucket_update_walker);
}

void
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


struct dht_s {
  struct dht_queue_t *queue;
  struct bucket_t *bucket;
  int (*send)(struct dht_s *dht, uint8_t *data, size_t length, uint8_t node_id[DHT_HASH_SIZE]);
};

dht_t *
dht_new(){
  dht_t *dht = calloc(1, sizeof(dht_t));
  if(dht == NULL) return NULL;

  dht->bucket = bucket_new(0, 255);
  if(dht->bucket == NULL) {
    free(dht);
    return NULL;
  }

  return dht;
}

void
dht_free(dht_t *dht) {
  free(dht);
}

int
dht_get(dht_t *dht, uint8_t *key, dht_get_callback cb, void *closure) {
  return 0;
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

struct _find_state {
  uint8_t target[32];
  node_t *current;
};

static int
_find_walker(void *ctx, bucket_t *root){
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

node_t *
dht_find_node(dht_t *dht, uint8_t key[32]) {
  if(dht->bucket->length == 0) return NULL;

  struct _find_state state;

  state.current = dht->bucket->nodes[0];
  memcpy(state.target, key, 32);

  bucket_walk((void *) &state, dht->bucket, _find_walker);

  return state.current;
}
