#include "iterablehash.h"

#include <string.h>
#include <stdlib.h>
#include <assert.h>

static unsigned long hash_function(const char *strinput) {
  unsigned long hash = 5381;
  const unsigned char *str = (const unsigned char *)strinput;
  int c;
  while ((c = *str++)) hash = hash * 33 ^ c;
  return hash;
}

void init_hash_table(struct hash_table *h, int size) {
  h->size = size;
  h->num_entries = 0;
  h->first = 0;
  h->table = calloc(sizeof(struct hash_entry *), size);
}

void free_hash_table(struct hash_table *h) {
  free(h->table); /* assume entries are already freed */
}

/* Find the data stored under str in the hash */
struct hash_entry * lookup_in_hash(struct hash_table *hash, const char *str) {
  unsigned long h = hash_function(str) % hash->size;
  struct hash_entry *e = hash->table[h];
  while (e) {
    if (strcmp(e->key, str) == 0) return e;
    if (!e->next || hash_function(e->next->key) % hash->size != h) return 0;
    e = e->next;
  }
  return 0;
}

/* Add the entry to the hash */
void add_to_hash(struct hash_table *hash, struct hash_entry *e) {
  assert(!lookup_in_hash(hash, e->key));
  hash->num_entries++;
  unsigned long h = hash_function(e->key) % hash->size;
  struct hash_entry *enext = hash->table[h];
  if (enext) {
    e->next = enext->next;
    enext->next = e;
  } else {
    e->next = hash->first;
    hash->first = e;
    hash->table[h] = e;
  }
}

void remove_from_hash(struct hash_table *hash, struct hash_entry *e) {
  assert(lookup_in_hash(hash, e->key));
  struct hash_entry *x = hash->first;
  if (x == e) {
    hash->first = e->next;
  } else {
    while (x) {
      if (x->next == e) {
        x->next = e->next;
      }
      x = x->next;
    }
  }
  unsigned long h = hash_function(e->key) % hash->size;
  if (hash->table[h] == e) {
    if (e->next && hash_function(e->next->key) % hash->size == h) {
      hash->table[h] = e->next;
    } else {
      hash->table[h] = 0;
    }
  }
  hash->num_entries -= 1;
}
