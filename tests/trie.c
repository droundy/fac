#include "../lib/trie.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *mycopy(const char *str) {
  char *out = malloc(strlen(str)+1);
  strcpy(out, str);
  return out;
}

struct trie *a = 0;

void has(const char *string, int line) {
  if (!lookup_in_trie(&a, string)) {
    printf("FAIL Has not %s (line %d)!!!\n", string, line);
    exit(1);
  }
}

void hasnot(const char *string, int line) {
  if (lookup_in_trie(&a, string)) {
    printf("FAIL Has %s (line %d)!!!\n", string, line);
    exit(1);
  }
}

void add(const char *string) {
  printf("adding %s\n", string);
  add_to_trie(&a, mycopy(string), "foo");
}

void remove_str(const char *string) {
  printf("removing %s\n", string);
  delete_from_trie(&a, string);
}

int main(int argc, char **argv) {
  hasnot("foo", __LINE__);
  remove_str("foo"); // shouldn't crash

  hasnot("foo", __LINE__);
  hasnot("foobar", __LINE__);
  hasnot("bar", __LINE__);
  hasnot("", __LINE__);

  add("foo");

  has("foo", __LINE__);
  hasnot("foobar", __LINE__);
  hasnot("bar", __LINE__);
  hasnot("", __LINE__);

  add("bar");

  has("foo", __LINE__);
  hasnot("foobar", __LINE__);
  has("bar", __LINE__);
  hasnot("", __LINE__);

  add("bar");

  has("foo", __LINE__);
  hasnot("foobar", __LINE__);
  has("bar", __LINE__);
  hasnot("", __LINE__);

  add("");

  has("foo", __LINE__);
  hasnot("foobar", __LINE__);
  has("bar", __LINE__);
  has("", __LINE__);

  add("foobar");

  has("foo", __LINE__);
  has("foobar", __LINE__);
  has("bar", __LINE__);
  has("", __LINE__);

  remove_str("bar");

  has("foo", __LINE__);
  has("foobar", __LINE__);
  hasnot("bar", __LINE__);
  has("", __LINE__);

  remove_str("foobar");

  has("foo", __LINE__);
  hasnot("foobar", __LINE__);
  hasnot("bar", __LINE__);
  has("", __LINE__);

  add("bar");

  has("foo", __LINE__);
  hasnot("foobar", __LINE__);
  has("bar", __LINE__);
  has("", __LINE__);

  remove_str("");

  has("foo", __LINE__);
  hasnot("foobar", __LINE__);
  has("bar", __LINE__);
  hasnot("", __LINE__);

  remove_str("foo");

  hasnot("foo", __LINE__);
  hasnot("foobar", __LINE__);
  has("bar", __LINE__);
  hasnot("", __LINE__);

  remove_str("bar");

  hasnot("foo", __LINE__);
  hasnot("foobar", __LINE__);
  hasnot("bar", __LINE__);
  hasnot("", __LINE__);

  return 0;
}
