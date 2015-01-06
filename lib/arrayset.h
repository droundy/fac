#ifndef ARRAYSET_H
#define ARRAYSET_H

#include <linux/limits.h>
#define ARRAY_MAX 4000000

typedef struct arrayset {
  int size;
  char a[ARRAY_MAX];
} arrayset;

void insert_to_arrayset(arrayset *array, const char *path);
void delete_from_arrayset(arrayset *array, const char *path);
int is_in_arrayset(const arrayset *array, const char *path);

void print_arrayset(const arrayset *array);

#endif
