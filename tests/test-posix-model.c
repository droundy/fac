#include "../lib/iterablehash.h"
#include "../lib/posixmodel.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int test_chdir(const char *dir, pid_t pid) {
  printf("%5d: chdir %s\n", pid, dir);
  if (model_chdir(0, dir, pid)) return 1;
  if (dir[0] == '/') {
    struct inode *cwd = model_cwd(pid);
    char *dname = model_realpath(cwd);
    printf("%5d: cwd -> %s\n", pid, dname);
    if (strcmp(dir, dname)) {
      printf("\nFAIL: %5d: %s != %s\n", pid, dname, dir);
      return 1;
    }
    free(dname);
  }
  return 0;
}

int main(int argc, char **argv) {
  pid_t pid = 100;
  int retval = test_chdir("/test/directory", pid);

  if (retval) {
    printf("Error: %d\n", retval);
  } else {
    printf("Success!\n");
  }
  return retval;
}
