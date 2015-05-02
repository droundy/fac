#ifndef HASHSET_H
#define HASHSET_H

#include "iterablehash.h"
#include <stdbool.h>

typedef struct hash_table hashset;

void initialize_hashset(hashset *array);
void free_hashset(hashset *h);

void insert_to_hashset(hashset *array, const char *path);
void delete_from_hashset(hashset *array, const char *path);
bool is_in_hashset(hashset *array, const char *path);

char **hashset_to_array(hashset *hs); // returns a single malloced array

#endif
