#include "trie.h"
#include <stdlib.h>

void ** place_in_trie(struct trie **trie, const char *str) {
  while (str[0]) {
    if (!*trie) {
      *trie = calloc(1, sizeof(struct trie));
    }
    trie = &(*trie)->children[(unsigned char)str[0]];
    str++;
  }
  if (!*trie) *trie = calloc(1, sizeof(struct trie));
  return &(*trie)->data;
}

void * lookup_in_trie(struct trie **trie, const char *str) {
  struct trie *t = *trie;
  while (str[0]) {
    if (!t) return 0;
    t = t->children[(unsigned char)str[0]];
    str++;
  }
  if (!t) return 0;
  return t->data;
}

void add_to_trie(struct trie **trie, const char *str, void *data) {
  while (str[0]) {
    if (!*trie) *trie = calloc(1, sizeof(struct trie));
    trie = &(*trie)->children[(unsigned char)str[0]];
    str++;
  }
  if (!*trie) *trie = calloc(1, sizeof(struct trie));
  (*trie)->data = data;
}

void delete_from_trie(struct trie **trie, const char *str) {
  struct trie *t = *trie;
  while (str[0]) {
    if (!t) return;
    t = t->children[(unsigned char)str[0]];
    str++;
  }
  if (t) t->data = 0;
}

void free_trie(struct trie **trie) {
  if (!*trie) return;
  for (int i=0;i<256;i++) free_trie(&(*trie)->children[i]);
  free(*trie);
  *trie = 0;
}
