#include "hashset.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

void initialize_hashset(hashset *hash) {
  init_hash_table(hash, 100);
}

void free_hashset(hashset *h) {
  free(h->table);
  struct hash_entry *todelete = 0;
  for (struct hash_entry *e = h->first; e; e = e->next) {
    free(todelete);
    todelete = e;
  }
  free(todelete);
}

void insert_to_hashset(hashset *hash, const char *path) {
  struct set_entry *e = (struct set_entry *)lookup_in_hash(hash, path);
  if (e) {
    e->is_valid = true;
    return;
  }
  e = malloc(sizeof(struct set_entry)+strlen(path)+1);
  strcpy(e->key, path);
  e->e.key = e->key;
  e->is_valid = true;
  add_to_hash(hash, &e->e);
}

void delete_from_hashset(hashset *hash, const char *path) {
  struct set_entry *e = (struct set_entry *)lookup_in_hash(hash, path);
  if (!e) return;
  e->is_valid = false; /* deletion should be rare */
}

bool is_in_hashset(hashset *hash, const char *path) {
  struct set_entry *e = (struct set_entry *)lookup_in_hash(hash, path);
  if (!e) return false;
  return e->is_valid;
}
