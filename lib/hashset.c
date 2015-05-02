#include "hashset.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

struct set_entry {
  struct hash_entry e;
  bool is_valid;
  char key[];
};

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

char **hashset_to_array(hashset *hs) {
  int numentries = 0;
  long total_size = 0;
  for (struct set_entry *e = (struct set_entry *)hs->first;
       e; e = (struct set_entry *)e->e.next) {
    if (e->is_valid) {
      numentries++;
      total_size += strlen(e->e.key) + 1;
    }
  }
  char **array = malloc((numentries + 1)*sizeof(char *) + total_size);
  char *strings = (char *)(array + numentries + 1);
  int i = 0;
  for (struct set_entry *e = (struct set_entry *)hs->first;
       e; e = (struct set_entry *)e->e.next) {
    if (e->is_valid) {
      array[i] = strings;
      const char *from = e->e.key;
      while (*from) {
        *strings++ = *from++; // copy the key and advance the string;
      }
      *strings++ = 0; // add the null termination
      i++;
    }
  }
  array[numentries] = 0; // terminate with a null pointer.
  return array;
}
