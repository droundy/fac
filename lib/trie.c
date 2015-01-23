#include "trie.h"
#include <stdlib.h>

void ** place_in_trie_pair(struct trie **trie, const char *str1, const char *str2) {
  while (str1[0]) {
    if (!*trie) *trie = calloc(1, sizeof(struct trie));
    trie = &(*trie)->children[(unsigned char)str1[0]];
    str1++;
  }
  if (str2) {
    if (!*trie) *trie = calloc(1, sizeof(struct trie));
    trie = &(*trie)->children[0];
    while (str2[0]) {
      if (!*trie) *trie = calloc(1, sizeof(struct trie));
      trie = &(*trie)->children[(unsigned char)str2[0]];
      str2++;
    }
  }
  if (!*trie) *trie = calloc(1, sizeof(struct trie));
  return &(*trie)->data;
}
void ** place_in_trie(struct trie **trie, const char *str) {
  return place_in_trie_pair(trie, str, 0);
}

void * lookup_in_trie_pair(struct trie **trie, const char *str1, const char *str2) {
  struct trie *t = *trie;
  while (str1[0]) {
    if (!t) return 0;
    t = t->children[(unsigned char)str1[0]];
    str1++;
  }
  if (str2) {
    if (!t) return 0;
    t = t->children[0];
    while (str2[0]) {
      if (!t) return 0;
      t = t->children[(unsigned char)str2[0]];
      str2++;
    }
  }
  if (!t) return 0;
  return t->data;
}
void * lookup_in_trie(struct trie **trie, const char *str) {
  return lookup_in_trie_pair(trie, str, 0);
}

void add_to_trie_pair(struct trie **trie, const char *str1, const char *str2, void *data) {
  while (str1[0]) {
    if (!*trie) *trie = calloc(1, sizeof(struct trie));
    trie = &(*trie)->children[(unsigned char)str1[0]];
    str1++;
  }
  if (str2) {
    if (!*trie) *trie = calloc(1, sizeof(struct trie));
    trie = &(*trie)->children[0];
    while (str2[0]) {
      if (!*trie) *trie = calloc(1, sizeof(struct trie));
      trie = &(*trie)->children[(unsigned char)str2[0]];
      str2++;
    }
  }
  if (!*trie) *trie = calloc(1, sizeof(struct trie));
  (*trie)->data = data;
}
void add_to_trie(struct trie **trie, const char *str, void *data) {
  add_to_trie_pair(trie, str, 0, data);
}

void delete_from_trie_pair(struct trie **trie, const char *str1, const char *str2) {
  struct trie *t = *trie;
  while (str1[0]) {
    if (!t) return;
    t = t->children[(unsigned char)str1[0]];
    str1++;
  }
  if (str2) {
    if (!t) return;
    t = t->children[0];
    while (str2[0]) {
      if (!t) return;
      t = t->children[(unsigned char)str2[0]];
      str2++;
    }
  }
  if (t) t->data = 0;
}
void delete_from_trie(struct trie **trie, const char *str) {
  delete_from_trie_pair(trie, str, 0);
}

void free_trie(struct trie **trie) {
  if (!*trie) return;
  for (int i=0;i<256;i++) free_trie(&(*trie)->children[i]);
  free(*trie);
  *trie = 0;
}
