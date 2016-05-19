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

static void init_intmap(struct intmap *m);
static void free_intmap(struct intmap *m, void (*free_datum)(void *));
static struct intmap *dup_intmap(struct intmap *m);

static void *lookup_intmap(struct intmap *m, int key);

static void add_to_intmap(struct intmap *m, int key, void *datum);
static void remove_intmapping(struct intmap *m, int key, void (*free_datum)(void *));

#endif
