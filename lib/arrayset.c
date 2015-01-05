#include "arrayset.h"

#include <error.h>
#include <string.h>

void insert_to_arrayset(arrayset *array, const char *path) {
  for (int i=array->num_elements-1;i>=0;i--) {
    if (strcmp(array->a[i], path) == 0) return;
  }

  if (array->num_elements >= ARRAY_SIZE) {
    error(1,0,"TOO MANY DEPENDENCIES!!!");
  }
  strncpy(array->a[array->num_elements], path, PATH_MAX);
  array->num_elements++;
}

void delete_from_arrayset(arrayset *array, const char *path) {
  for (int i=array->num_elements-1;i>=0;i--) {
    if (strcmp(array->a[i], path) == 0) {
      memmove(&array->a[i], &array->a[i+1], PATH_MAX*(array->num_elements-i-1));
      array->num_elements--;
      return;
    }
  }
}

int is_in_arrayset(const arrayset *array, const char *path) {
  for (int i=0;i<array->num_elements;i++) {
    if (strcmp(array->a[i], path) == 0) {
      return 1;
    }
  }
  return 0;
}
