#ifndef HASHSET_H
#define HASHSET_H

#include "iterablehash.h"
#include <stdbool.h>

struct set_entry {
  struct hash_entry e;
  bool is_valid;
  char key[];
};

typedef struct hash_table hashset;

void initialize_hashset(hashset *array);
void free_hashset(hashset *h);

void insert_to_hashset(hashset *array, const char *path);
void delete_from_hashset(hashset *array, const char *path);
bool is_in_hashset(hashset *array, const char *path);

#endif
