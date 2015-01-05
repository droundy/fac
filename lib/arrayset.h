#ifndef ARRAYSET_H
#define ARRAYSET_H

#include <linux/limits.h>
#define ARRAY_SIZE 4096;

typedef struct arrayset {
  int num_elements;
  char a[ARRAYSET][PATH_MAX];
} arrayset;

void insert_to_arrayset(arrayset *array, const char *path);
void delete_from_arrayset(arrayset *array, const char *path);
int is_in_arrayset(const arrayset *array, const char *path);

#endif
