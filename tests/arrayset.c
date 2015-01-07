#include "../lib/arrayset.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

/* void assert(int x) { */
/*   if (!x) exit(1); */
/* } */

struct arrayset *a = 0;

void has(const char *string, int line) {
  if (!is_in_arrayset(a, string)) {
    printf("FAIL Has not %s (line %d)!!!\n", string, line);
    exit(1);
  }
}

void hasnot(const char *string, int line) {
  if (is_in_arrayset(a, string)) {
    printf("FAIL Has %s (line %d)!!!\n", string, line);
    exit(1);
  }
}

void testiter(int line) {
  for (char *v = start_iterating(a); v; v = iterate(a)) {
    has(v, line);
  }
}

int main(int argc, char **argv) {
  printf("This is silly!\n");

  a = malloc(sizeof(struct arrayset));
  initialize_arrayset(a);

  testiter(__LINE__);

  hasnot("foo", __LINE__);
  delete_from_arrayset(a, "foo"); // shouldn't crash

  hasnot("foo", __LINE__);
  hasnot("foobar", __LINE__);
  hasnot("bar", __LINE__);
  hasnot("", __LINE__);

  insert_to_arrayset(a, "foo");
  testiter(__LINE__);

  has("foo", __LINE__);
  hasnot("foobar", __LINE__);
  hasnot("bar", __LINE__);
  hasnot("", __LINE__);

  insert_to_arrayset(a, "bar");
  testiter(__LINE__);

  has("foo", __LINE__);
  hasnot("foobar", __LINE__);
  has("bar", __LINE__);
  hasnot("", __LINE__);

  insert_to_arrayset(a, "bar");
  testiter(__LINE__);

  printf("\nJust added bar\n");
  print_arrayset(a);

  has("foo", __LINE__);
  hasnot("foobar", __LINE__);
  has("bar", __LINE__);
  hasnot("", __LINE__);

  insert_to_arrayset(a, "");
  testiter(__LINE__);

  printf("\nJust added \"\"\n");
  print_arrayset(a);

  has("foo", __LINE__);
  hasnot("foobar", __LINE__);
  has("bar", __LINE__);
  has("", __LINE__);

  insert_to_arrayset(a, "foobar");

  printf("\nJust added foobar\n");
  print_arrayset(a);

  has("foo", __LINE__);
  has("foobar", __LINE__);
  has("bar", __LINE__);
  has("", __LINE__);

  delete_from_arrayset(a, "bar");

  printf("\nJust deleted bar\n");
  print_arrayset(a);

  has("foo", __LINE__);
  has("foobar", __LINE__);
  hasnot("bar", __LINE__);
  has("", __LINE__);

  delete_from_arrayset(a, "foobar");

  printf("\nJust deleted foobar\n");
  print_arrayset(a);

  has("foo", __LINE__);
  hasnot("foobar", __LINE__);
  hasnot("bar", __LINE__);
  has("", __LINE__);

  insert_to_arrayset(a, "bar");

  printf("\nJust added bar\n");
  print_arrayset(a);

  has("foo", __LINE__);
  hasnot("foobar", __LINE__);
  has("bar", __LINE__);
  has("", __LINE__);

  delete_from_arrayset(a, "");

  printf("\nJust deleted ''\n");
  print_arrayset(a);

  has("foo", __LINE__);
  hasnot("foobar", __LINE__);
  has("bar", __LINE__);
  hasnot("", __LINE__);

  delete_from_arrayset(a, "foo");

  printf("\nJust deleted foo\n");
  print_arrayset(a);

  hasnot("foo", __LINE__);
  hasnot("foobar", __LINE__);
  has("bar", __LINE__);
  hasnot("", __LINE__);

  delete_from_arrayset(a, "bar");

  printf("\nJust deleted bar\n");
  print_arrayset(a);

  hasnot("foo", __LINE__);
  hasnot("foobar", __LINE__);
  hasnot("bar", __LINE__);
  hasnot("", __LINE__);

  return 0;
}
