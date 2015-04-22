#include "intmap.h"

#include <stdlib.h>

void init_intmap(struct intmap *m) {
  m->size = 128;
  m->num_ints = 0;
  m->table = malloc(m->size*sizeof(struct intmap_entry));
  for (int i=0;i<m->size;i++) m->table[i].whichdatum = -1;
  m->data_size = 0;
  m->data = 0;
}

void free_intmap(struct intmap *m, void (*free_datum)(void *)) {
  for (int i=0;i<m->data_size;i++) {
    if (m->data[i]) free_datum(m->data[i]);
  }
  free(m->table);
}

static void place_whichdatum(struct intmap *m, int key, int whichdatum) {
  int h = key % m->size;
  while (m->table[h].whichdatum != -1 && m->table[h].key != key) {
    h++;
  }
  m->table[h].whichdatum = whichdatum;
  m->table[h].key = key;
}

void add_to_intmap(struct intmap *m, int key, void *datum) {
  int whichdatum = -1;
  for (int i=0;i<m->data_size;i++) {
    if (!m->data[i]) {
      whichdatum = i;
      break;
    }
  }
  if (whichdatum == -1) {
    whichdatum = m->data_size;
    m->data_size += 1;
    m->data = realloc(m->data, m->data_size*sizeof(void *));
  }
  m->data[whichdatum] = datum;
  if (m->num_ints > m->size/2) {
    struct intmap_entry *old_table = m->table;
    const int oldsize = m->size;
    m->size *= 2;
    m->table = malloc(m->size*sizeof(struct intmap_entry));
    for (int i=0;i<m->size;i++) m->table[i].whichdatum = -1;
    for (int i=0;i<oldsize;i++) {
      place_whichdatum(m, old_table[i].key, old_table[i].whichdatum);
    }
  }
  place_whichdatum(m, key, whichdatum);
}

void *lookup_intmap(struct intmap *m, int key) {
  int h = key % m->size;
  while (m->table[h].whichdatum != -1 && m->table[h].key != key) {
    h++;
  }
  if (m->table[h].whichdatum != -1 && m->table[h].key == key) {
    return m->data[m->table[h].whichdatum];
  }
  return 0;
}

void remove_intmapping(struct intmap *m, int key, void (*free_datum)(void *)) {
}

