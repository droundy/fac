#ifndef ARRAYSET_H
#define ARRAYSET_H

#include <linux/limits.h>
#define ARRAY_MAX 4000000

typedef struct arrayset {
  int size;
  int iterator;
  char a[ARRAY_MAX];
} arrayset;

void initialize_arrayset(arrayset *array);

void insert_to_arrayset(arrayset *array, const char *path);
void delete_from_arrayset(arrayset *array, const char *path);
int is_in_arrayset(const arrayset *array, const char *path);

void print_arrayset(const arrayset *array);

char *start_iterating(arrayset *array);
char *iterate(arrayset *array);

#endif
