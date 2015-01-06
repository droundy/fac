#include <stdlib.h>
#include <string.h>

#include "listset.h"

void delete_from_listset(listset **list, const char *path) {
  while (*list != NULL) {
    if (strcmp((*list)->path, path) == 0) {
      listset *to_be_deleted = *list;
      *list = (*list)->next;
      free(to_be_deleted->path);
      free(to_be_deleted);
      return;
    }
    list = &((*list)->next);
  }
}

int is_in_listset(const listset *ptr, const char *path) {
  while (ptr != NULL) {
    if (strcmp(ptr->path, path) == 0) {
      return 1;
    }
    ptr = ptr->next;
  }
  return 0;
}

void insert_to_listset(listset **list, const char *path) {
  delete_from_listset(list, path);
  listset *newhead = (listset *)malloc(sizeof(listset));
  newhead->next = *list;
  newhead->path = malloc(strlen(path)+1);
  strcpy(newhead->path, path);

  *list = newhead;
}

void free_listset(listset *list) {
  while (list != NULL) {
    listset *d = list;
    list = list->next;
    free(d->path);
    free(d);
  }
}
