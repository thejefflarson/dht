#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include "dht_bucket.h"

static bool
_contains(dht_bucket_t *root, dht_node_t *node){
  return root->upper_limit > (uint8_t) node->id[0] && root->lower_limit <= (uint8_t) node->id[0];
}

static bool
_has_space(dht_bucket_t *root){
  return root->length < 8;
}

static uint8_t
_mid(dht_bucket_t* root){
  return (root->upper_limit - root->lower_limit) / 2 + root->lower_limit;
}

static bool
_split(dht_bucket_t *root){
  if((root->upper_limit - root->lower_limit) == 1)
    return true; // can't split anymore

  uint8_t mid = _mid(root);

  dht_bucket_t *next = dht_bucket_new(mid, root->upper_limit);
  if(next == NULL)
    return true;

  next->next = root->next;
  root->next = next;
  root->upper_limit = mid;

  for(int i = 0; i < root->length; i++){
    if(!_contains(root, root->nodes[i])) {
      if(dht_bucket_insert(next, root->nodes[i])) {
        if(i + 1 < root->length)
          memmove(root->nodes + i, root->nodes + i + 1, sizeof(dht_node_t *) * (root->length - i - 1));
        i--;
        root->length--;
      }
    }
  }

  return false;
}

dht_bucket_t*
dht_bucket_new(uint8_t lower, uint8_t upper) {
  dht_bucket_t* bucket = calloc(1, sizeof(dht_bucket_t));
  if(bucket == NULL)
    return NULL;
  bucket->upper_limit = upper;
  bucket->lower_limit = lower;
  bucket->next = NULL;
  return bucket;
}

dht_bucket_t*
dht_bucket_insert(dht_bucket_t *root, dht_node_t *node) {
  while(root != NULL && !_contains(root, node))
    root = root->next;

  if(root == NULL){
    return NULL; // shouldn't happen
  }

  if(!_has_space(root)) {
    bool err = _split(root);
    if(err) return NULL;
  }

  // have to check again to see if some nodes moved over
  if(_has_space(root)) {
    root->nodes[root->length++] = node;
    dht_bucket_update(root);
  } else {
    return NULL;
  }

  return root;
}

void
dht_bucket_walk(void *ctx, dht_bucket_t *root, dht_bucket_walk_callback cb) {
  if(cb(ctx, root) == 0 && root->next != NULL)
    dht_bucket_walk(ctx, root->next, cb);
}

static int
_update_walker(void *ctx, dht_bucket_t *root){
  (void) ctx;
  for(int i = 0; i < root->length; i++){
    if(!dht_node_good(root->nodes[i])){
      dht_node_free(root->nodes[i]);
      if(i + 1 < root->length)
        memmove(root->nodes + i, root->nodes + i + 1, sizeof(dht_node_t *) * (root->length - i - 1));
      i--;
      root->length--;
    }
  }
  return 0;
}

void
dht_bucket_update(dht_bucket_t *root) {
  dht_bucket_walk(NULL, root, _update_walker);
}

void
dht_bucket_free(dht_bucket_t *root) {
  dht_bucket_t *b;
  while(root != NULL) {
    for(int i = 0; i < root->length; i++)
      dht_node_free(root->nodes[i]);
    b = root;
    root = b->next;
    free(b);
  }
}
