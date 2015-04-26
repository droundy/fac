#include "intmap.h"

#include <stdlib.h>
#include <string.h>

void init_intmap(struct intmap *m) {
  m->size = 128;
  m->num_ints = 0;
  m->table = malloc(m->size*sizeof(struct intmap_entry));
  for (int i=0;i<m->size;i++) m->table[i].whichdatum = -1;
  m->data_size = 0;
  m->data = 0;
}

struct intmap *dup_intmap(struct intmap *m) {
  struct intmap *o = malloc(sizeof(struct intmap));
  o->size = m->size;
  o->num_ints = m->num_ints;
  o->data_size = m->data_size;
  o->table = malloc(o->size*sizeof(struct intmap_entry));
  o->data = malloc(o->data_size*sizeof(void *));
  memcpy(o->data, m->data, o->data_size*sizeof(void *));
  memcpy(o->table, m->table, o->size*sizeof(struct intmap_entry));
  return o;
}

void free_intmap(struct intmap *m, void (*free_datum)(void *)) {
  if (free_datum) {
    for (int i=0;i<m->data_size;i++) {
      if (m->data[i]) {
        for (int j=i+1;j<m->data_size;j++) {
          if (m->data[j] == m->data[i]) m->data[j] = 0;
        }
        free_datum(m->data[i]);
      }
    }
  }
  free(m->table);
  free(m->data);
}

static void place_whichdatum(struct intmap *m, int key, int whichdatum) {
  unsigned int h = (unsigned int)key % m->size;
  while (m->table[h].whichdatum != -1 && m->table[h].key != key) {
    h++;
  }
  m->table[h].whichdatum = whichdatum;
  m->table[h].key = key;
  m->num_ints += 1;
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
  if (m->num_ints > m->size/2 || m->num_ints + 128 < (m->size/8)) {
    struct intmap_entry *old_table = m->table;
    const int oldsize = m->size;
    m->size = (m->num_ints + 1)*4;
    m->num_ints = 0;
    m->table = malloc(m->size*sizeof(struct intmap_entry));
    for (int i=0;i<m->size;i++) m->table[i].whichdatum = -1;
    for (int i=0;i<oldsize;i++) {
      if (old_table[i].whichdatum != -1) {
        place_whichdatum(m, old_table[i].key, old_table[i].whichdatum);
      }
    }
    free(old_table);
  }
  place_whichdatum(m, key, whichdatum);
}

void *lookup_intmap(struct intmap *m, int key) {
  unsigned int h = (unsigned int)key % m->size;
  while (m->table[h].whichdatum != -1 && m->table[h].key != key) {
    h++;
  }
  if (m->table[h].whichdatum != -1 && m->table[h].key == key) {
    return m->data[m->table[h].whichdatum];
  }
  return 0;
}

void remove_intmapping(struct intmap *m, int key, void (*free_datum)(void *)) {
  unsigned int h = (unsigned int)key % m->size;
  while (m->table[h].whichdatum != -1 && m->table[h].key != key) {
    h++;
  }
  if (m->table[h].whichdatum != -1 && m->table[h].key == key) {
    if (free_datum) {
      free_datum(m->data[m->table[h].whichdatum]);
    }
    m->data[m->table[h].whichdatum] = 0;
    m->table[h].whichdatum = -1;
    m->num_ints -= 1;
  }
}

