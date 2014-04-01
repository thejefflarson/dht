#include <stdlib.h>
#include "dht_bucket.h"


bool
_contains(dht_bucket_t *root, dht_node_t *node){
  return root->upper_limit >= (uint8_t) node->id[0] && root->lower_limit =< (uint8_t) node->id[0];
}

bool
_has_space(dht_bucket_t *root){
  return (root->length < 8 || ((root->lower != NULL) && (root->upper != NULL)));
}

uint8_t
_mid(dht_bucket_t* root){
  return (root->upper_limit - root->lower_limit) / 2;
}

bool
_split(dht_bucket_t *root){
  uint8_t mid = _mid(root);
  root->upper = dht_bucket_new(mid, root->upper_limit);
  root->lower = dht_bucket_new(mid, root->upper_limit);
  if(root->upper == NULL || root->lower == NULL) return FALSE;
  for(int i = 0; i < root->length; i++){
    dht_bucket_t *buck = dht_bucket_insert(root, root->nodes[i]);
    if(buck == NULL) return FALSE;
    root->nodes[i] = NULL;
  }
  return TRUE;
}

dht_bucket_t*
dht_bucket_new(uint8_t lower, uint8_t upper) {
  dht_bucket_t* bucket;
  if((bucket = malloc(sizeof(dht_bucket_t))) == NULL)
    return NULL;
  memset(bucket, 0, sizeof(dht_bucket_t));
  bucket->upper_limit = upper;
  bucket->lower_limit = lower;
  return bucket;
}

dht_bucket_t*
dht_bucket_insert(dht_bucket_t *root, dht_node_t *node) {
  if(!_contains(root, node)) return NULL; // shouldn't happen
  if(_has_space(root)) {
    root->nodes[root->length++] = node;
    dht_bucket_update(root);
    return root;
  } else {
    bool err = _split(root);
    if(err) return NULL;
  }
  if((uint8_t) node->id[0] > (root->upper - root->lower) / 2) {
    return dht_bucket_insert(root->upper, node);
  } else {
    return dht_bucket_insert(root->lower, node);
  }
}

void
dht_bucket_walk(void *ctx, dht_bucket_t *root, dht_bucket_walk_callback *cb) {
  if(root->left && root->right) {
    int stop = cb(ctx, root->left) || cb(ctx, root->right);
    if(stop == 0)
      dht_bucket_walk(root, ctx, cb);
  }
}

int
_compare(const void* A, const void* B){
  dht_node_t a = A;
  dht_node_t b = B;
  if(a->created_at < b->created_at) {
    return 1;
  } else {
    return -1;
  }
}

int
_update_walker(void *ctx, dht_bucket_t *root){
  for(int i = 0; i < root->length; i++){
    if(node->last_heard > 15 * 60 * 100){
      dht_bucket_remove_node(root, root->nodes[i]->id);
      i--;
    }
  }
  qsort(root->nodes, root->length, sizeof(dht_node_t), _compare);
  return 0;
}

void
dht_bucket_update(dht_bucket_t *root) {
  dht_update_walk(NULL, root, _update_walker);
}

void
dht_bucket_free(dht_bucket_t *root) {
  if(root->left && root->right){
    dht_bucket_free(root->left);
    dht_bucket_free(root->right);
  }
  for(int i = 0; i < root->length; i++){
    dht_node_free(root->nodes[i]);
  }
  free(root);
}
