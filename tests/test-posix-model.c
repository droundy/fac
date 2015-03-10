#include "../lib/iterablehash.h"
#include "../lib/posixmodel.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv) {
  pid_t pid = 100;
  int retval = model_chdir(0, "/test/directory", pid);

  struct inode *cwd = model_cwd(pid);
  char *dname = model_realpath(cwd);
  printf("cwd gives %s\n", dname);
  if (strcmp("/test/directory", dname)) {
    retval |= 1;
    printf("\nFAIL: %s should be %s\n", dname, "/test/directory");
  }
  free(dname);

  if (retval) {
    printf("Error: %d\n", retval);
  } else {
    printf("Success!\n");
  }
  return retval;
}
