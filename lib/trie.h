#ifndef TRIE_H
#define TRIE_H

/* This trie is *not* memory-saving, since it takes a minimum of 2k
   bytes per string, and in most cases about 2k*string length! But it
   does handle any possible string, and lookups scale with string
   length, and are independent of the number of strings. */
struct trie {
  struct trie *children[256];
  void *data;
};

/* Use place_in_trie to delay creating your data if you want to add it
   only after checking whether the data is already present.  This is
   marginally faster than lookup_in_trie followed by add_to_trie. */
void ** place_in_trie(struct trie **trie, const char *str);

/* Find the data stored under str in the trie */
void * lookup_in_trie(struct trie **trie, const char *str);

/* Add the data stored under str in the trie */
void add_to_trie(struct trie **trie, const char *str, void *data);

/* Delete the data stored under str in the trie.  Doesn't actually
   free any other memory. */
void delete_from_trie(struct trie **trie, const char *str);

#endif
