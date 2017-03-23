#include "../lib/iterablehash.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

struct hash_table a;

void has(const char *string, int line) {
  if (!lookup_in_hash(&a, string)) {
    printf("FAIL Has not %s (line %d)!!!\n", string, line);
    exit(1);
  }
  if (lookup_in_hash(&a, string)->key != string) {
    printf("FAIL has wrong pointer %s (line %d)!!!\n", string, line);
    exit(1);
  }
}

void hasnot(const char *string, int line) {
  if (lookup_in_hash(&a, string)) {
    printf("FAIL Has %s (line %d)!!!\n", string, line);
    exit(1);
  }
}

void add(const char *string) {
  printf("adding %s\n", string);
  struct hash_entry *e = malloc(sizeof(struct hash_entry));
  e->key = string;
  add_to_hash(&a, e);
}

int main(int argc, char **argv) {
  init_hash_table(&a, 5);

  assert(a.num_entries == 0);

  hasnot("foo", __LINE__);
  hasnot("foobar", __LINE__);
  hasnot("bar", __LINE__);
  hasnot("", __LINE__);

  add("foo");

  assert(a.num_entries == 1);

  has("foo", __LINE__);
  hasnot("foobar", __LINE__);
  hasnot("bar", __LINE__);
  hasnot("", __LINE__);

  add("bar");

  assert(a.num_entries == 2);

  has("foo", __LINE__);
  hasnot("foobar", __LINE__);
  has("bar", __LINE__);
  hasnot("", __LINE__);

  {
    int count_entries = 0;
    for (struct hash_entry *e = a.first; e; e = e->next) {
      count_entries++;
    }
    assert(count_entries == a.num_entries);
  }

  add("");

  assert(a.num_entries == 3);

  has("foo", __LINE__);
  hasnot("foobar", __LINE__);
  has("bar", __LINE__);
  has("", __LINE__);

  add("foobar");

  assert(a.num_entries == 4);

  has("foo", __LINE__);
  has("foobar", __LINE__);
  has("bar", __LINE__);
  has("", __LINE__);

  {
    int count_entries = 0;
    for (struct hash_entry *e = a.first; e; e = e->next) {
      count_entries++;
    }
    assert(count_entries == a.num_entries);
  }

  return 0;
}
