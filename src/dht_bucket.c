#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

#include "dht_bucket.h"


bool
_contains(dht_bucket_t *root, dht_node_t *node){
  return root->upper_limit >= (uint8_t) node->id[0] && root->lower_limit <= (uint8_t) node->id[0];
}

bool
_has_space(dht_bucket_t *root){
  return root->length < 8;
}

uint8_t
_mid(dht_bucket_t* root){
  return (root->upper_limit - root->lower_limit) / 2;
}

bool
_split(dht_bucket_t *root){
  uint8_t mid = _mid(root);
  if(mid == root->upper_limit) return false; // can't split anymore
  dht_bucket_t *next = dht_bucket_new(mid, root->upper_limit);
  if(next == NULL) return false;
  next->next = root->next;
  root->next = next;
  root->upper_limit = mid;

  for(int i = 0; i < root->length; i++){
    if(!_contains(root, root->nodes[i])) {
      dht_bucket_insert(next, root->nodes[i]);
      root->nodes[i] = NULL;
      root->length--;
    }
  }

  return true;
}

dht_bucket_t*
dht_bucket_new(uint8_t lower, uint8_t upper) {
  dht_bucket_t* bucket;
  if((bucket = malloc(sizeof(dht_bucket_t))) == NULL)
    return NULL;
  memset(bucket, 0, sizeof(dht_bucket_t));
  bucket->upper_limit = upper;
  bucket->lower_limit = lower;
  bucket->next = NULL;
  return bucket;
}

dht_bucket_t*
dht_bucket_insert(dht_bucket_t *root, dht_node_t *node) {
  dht_bucket_t *buck = root;

  while(buck != NULL && !_contains(buck, node))
    buck = buck->next;

  if(buck == NULL){
    return NULL; // shouldn't happen
  }

  if(_has_space(buck)) {
    buck->nodes[buck->length++] = node;
    dht_bucket_update(buck);
    return buck;
  } else {
    bool err = _split(buck);
    if(err) return NULL;
    return dht_bucket_insert(buck, node);
  }
}

void
dht_bucket_walk(void *ctx, dht_bucket_t *root, dht_bucket_walk_callback cb) {
  if(cb(ctx, root) == 0 && root->next != NULL)
    dht_bucket_walk(ctx, root->next, cb);
}

int
_compare(const void* A, const void* B){
  const dht_node_t* a = A;
  const dht_node_t* b = B;
  if(a == NULL) {
    return -1;
  } else if(b == NULL) {
    return 1;
  } else if(a->created_at < b->created_at) {
    return 1;
  } else {
    return -1;
  }
}

int
_update_walker(void *ctx, dht_bucket_t *root){
  (void) ctx;
  while(current->next != NULL) {
    if(!dht_node_good(current)){

      dht_node_free(current);
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
    for(int i = 0; i < 8; i++){
      if(root->nodes[i] != NULL){
        dht_node_free(root->nodes[i]);
        root->nodes[i] = NULL;
      }
    }
    b = root;
    root = b->next;
    free(b);
  }
}
