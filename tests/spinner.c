#include <time.h>
#include <stdio.h>

int main(int argc, char ** argv) {
  clock_t ticks = 10*CLOCKS_PER_SEC;
  if (argc == 2) {
    double seconds = 10;
    sscanf(argv[1], "%lg", &seconds);
    ticks = seconds*CLOCKS_PER_SEC;
  }
  printf("Starting timing for %lg seconds...\n", ticks/(double)CLOCKS_PER_SEC);
  while (clock() < ticks);
  printf("All done spinning!\n");
  return 0;
}

