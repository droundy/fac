#ifndef HASHSET_H
#define HASHSET_H

#include "iterablehash.h"

#include <string.h>

typedef struct hash_table hashset;

static inline void insert_hashset(hashset *hash, const char *key) {
  if (lookup_in_hash(hash, key)) return;
  struct hash_entry *e = malloc(sizeof(struct hash_entry)+strlen(key)+1);
  e->key = ((char *)e) + sizeof(struct hash_entry);
  strcpy((char *)e->key, key);
  e->next = 0;
  add_to_hash(hash, e);
}

static inline void delete_from_hashset(hashset *hash, const char *key) {
  struct hash_entry *e = lookup_in_hash(hash, key);
  if (e) {
    remove_from_hash(hash, e);
    free(e);
  }
}

static char **hashset_to_array(hashset *hs) {
  int numentries = 0;
  long total_size = 0;
  for (struct hash_entry *e = (struct hash_entry *)hs->first;
       e; e = (struct hash_entry *)e->next) {
    numentries++;
    total_size += strlen(e->key) + 1;
  }
  char **array = malloc((numentries + 1)*sizeof(char *) + total_size);
  char *strings = (char *)(array + numentries + 1);
  int i = 0;
  for (struct hash_entry *e = (struct hash_entry *)hs->first;
       e; e = (struct hash_entry *)e->next) {
    array[i] = strings;
    const char *from = e->key;
    while (*from) {
      *strings++ = *from++; // copy the key and advance the string;
    }
    *strings++ = 0; // add the null termination
    i++;
  }
  array[numentries] = 0; // terminate with a null pointer.
  return array;
}

static void free_hashset(hashset *h) {
  free_hash_table(h);
  struct hash_entry *todelete = 0;
  for (struct hash_entry *e = h->first; e; e = e->next) {
    free(todelete);
    todelete = e;
  }
  free(todelete);
}

#endif
