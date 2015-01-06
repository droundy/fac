#include "../lib/arrayset.h"

#include <stdio.h>
#include <stdlib.h>
#include <error.h>

int main(int argc, char **argv) {
  printf("This is silly!\n");

  struct arrayset *a = malloc(sizeof(struct arrayset));
  if (is_in_arrayset(a, "foo"))
    error_at_line(1, 0, __FILE__, __LINE__, "foo shouldn't be here.");
  delete_from_arrayset(a, "foo"); // shouldn't crash

  insert_to_arrayset(a, "foo");

  if (is_in_arrayset(a, "bar"))
    error_at_line(1, 0, __FILE__, __LINE__, "bar shouldn't be here.");
  if (is_in_arrayset(a, "bar"))
    error_at_line(1, 0, __FILE__, __LINE__, "bar shouldn't be here.");

  return 0;
}
