#include "../lib/hashset.h"

#include <stdio.h>
#include <stdlib.h>

hashset *a = 0;

void has(const char *string, int line) {
  if (!is_in_hashset(a, string)) {
    printf("FAIL Has not %s (line %d)!!!\n", string, line);
    exit(1);
  }
}

void hasnot(const char *string, int line) {
  if (is_in_hashset(a, string)) {
    printf("FAIL Has %s (line %d)!!!\n", string, line);
    exit(1);
  }
}

void testiter(int line) {
  for (struct set_entry *e = (struct set_entry *)a->first; e; e = (struct set_entry *)e->e.next) {
    if (e->is_valid) has(e->key, line);
  }
}

int main(int argc, char **argv) {
  a = malloc(sizeof(hashset));
  initialize_hashset(a);

  testiter(__LINE__);

  hasnot("foo", __LINE__);
  delete_from_hashset(a, "foo"); // shouldn't crash

  hasnot("foo", __LINE__);
  hasnot("foobar", __LINE__);
  hasnot("bar", __LINE__);
  hasnot("", __LINE__);

  insert_to_hashset(a, "foo");
  testiter(__LINE__);

  has("foo", __LINE__);
  hasnot("foobar", __LINE__);
  hasnot("bar", __LINE__);
  hasnot("", __LINE__);

  insert_to_hashset(a, "bar");
  testiter(__LINE__);

  has("foo", __LINE__);
  hasnot("foobar", __LINE__);
  has("bar", __LINE__);
  hasnot("", __LINE__);

  insert_to_hashset(a, "bar");
  testiter(__LINE__);

  printf("\nJust added bar\n");
  //print_hashset(a);

  has("foo", __LINE__);
  hasnot("foobar", __LINE__);
  has("bar", __LINE__);
  hasnot("", __LINE__);

  insert_to_hashset(a, "");
  testiter(__LINE__);

  printf("\nJust added \"\"\n");
  //print_hashset(a);

  has("foo", __LINE__);
  hasnot("foobar", __LINE__);
  has("bar", __LINE__);
  has("", __LINE__);

  insert_to_hashset(a, "foobar");

  printf("\nJust added foobar\n");
  //print_hashset(a);

  has("foo", __LINE__);
  has("foobar", __LINE__);
  has("bar", __LINE__);
  has("", __LINE__);

  delete_from_hashset(a, "bar");

  printf("\nJust deleted bar\n");
  //print_hashset(a);

  has("foo", __LINE__);
  has("foobar", __LINE__);
  hasnot("bar", __LINE__);
  has("", __LINE__);

  delete_from_hashset(a, "foobar");

  printf("\nJust deleted foobar\n");
  //print_hashset(a);

  has("foo", __LINE__);
  hasnot("foobar", __LINE__);
  hasnot("bar", __LINE__);
  has("", __LINE__);

  insert_to_hashset(a, "bar");

  printf("\nJust added bar\n");
  //print_hashset(a);

  has("foo", __LINE__);
  hasnot("foobar", __LINE__);
  has("bar", __LINE__);
  has("", __LINE__);

  delete_from_hashset(a, "");

  printf("\nJust deleted ''\n");
  //print_hashset(a);

  has("foo", __LINE__);
  hasnot("foobar", __LINE__);
  has("bar", __LINE__);
  hasnot("", __LINE__);

  delete_from_hashset(a, "foo");

  printf("\nJust deleted foo\n");
  //print_hashset(a);

  hasnot("foo", __LINE__);
  hasnot("foobar", __LINE__);
  has("bar", __LINE__);
  hasnot("", __LINE__);

  delete_from_hashset(a, "bar");

  printf("\nJust deleted bar\n");
  //print_hashset(a);

  hasnot("foo", __LINE__);
  hasnot("foobar", __LINE__);
  hasnot("bar", __LINE__);
  hasnot("", __LINE__);

  return 0;
}
