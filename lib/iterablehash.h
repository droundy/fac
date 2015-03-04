#ifndef ITERABLEHASH_H
#define ITERABLEHASH_H

struct hash_entry {
  const char *key;
  struct hash_entry *next;
};

struct hash_table {
  int size, num_entries;
  struct hash_entry *first;
  struct hash_entry **table;
};

void init_hash_table(struct hash_table *h, int size);
void free_hash_table(struct hash_table *h);

/* Find the data stored under str in the hash */
struct hash_entry * lookup_in_hash(struct hash_table *hash, const char *str);

/* Add the entry to the hash */
void add_to_hash(struct hash_table *hash, struct hash_entry *e);
void remove_from_hash(struct hash_table *hash, struct hash_entry *e);

#endif
