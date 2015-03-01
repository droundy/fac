#include <stdio.h>
#include <assert.h>

int main(int argc, char **argv) {
  int i;
  printf("This is a test.");
  for (i=1;i<10000;i++) {
    printf("counting %d...\n", i);
    fflush(stdout);
    assert(i % 999);
  }
  return 0;
}
