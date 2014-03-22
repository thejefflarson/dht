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
    dht_bucket_t *buck = dht_bucket_insert(root, root->nodes[i]) == NULL)
    root->nodes[i] = NULL;
    if(buck == NULL) return FALSE;
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
  if(!_contains(root,node)) return NULL; // shouldn't happen
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
dht_bucket_walk(dht_bucket_t *root, dht_baton_t *baton) {

}

void
dht_bucket_update(dht_bucket_t *root) {

}

void
dht_bucket_free(dht_bucket_t *root) {

}
