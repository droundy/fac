#ifndef INTMAP_H
#define INTMAP_H

struct intmap_entry {
  int key, whichdatum;
};

struct intmap {
  unsigned int size, num_ints;
  struct intmap_entry *table;
  int data_size;
  void **data;
};

void init_intmap(struct intmap *m);
void free_intmap(struct intmap *m, void (*free_datum)(void *));
struct intmap *dup_intmap(struct intmap *m);

void *lookup_intmap(struct intmap *m, int key);

void add_to_intmap(struct intmap *m, int key, void *datum);
void remove_intmapping(struct intmap *m, int key, void (*free_datum)(void *));

#endif
