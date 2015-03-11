#define _XOPEN_SOURCE 700

#include "../lib/iterablehash.h"
#include "../lib/posixmodel.h"

#include <unistd.h>
#include <sys/stat.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

int verify_cwd(const char *dir_expected, pid_t pid) {
  struct inode *cwd = model_cwd(pid);
  char *dname = model_realpath(cwd);
  printf("%5d: cwd -> %s\n", pid, dname);
  if (strcmp(dir_expected, dname)) {
    printf("\nFAIL: %5d: %s != %s\n", pid, dname, dir_expected);
    return 1;
  }
  free(dname);
  return 0;
}

int test_chdir(const char *dir, pid_t pid) {
  printf("%5d: chdir %s\n", pid, dir);
  if (model_chdir(model_cwd(pid), dir, pid)) return 1;
  if (dir[0] == '/') verify_cwd(dir, pid);
  return 0;
}

void attempt(int err) {
  if (err) {
    printf("FAIL!\n");
    exit(1);
  }
}

void attempt_errno(int err, const char *msg) {
  if (err) {
    printf("FAIL! %s: %s\n", msg, strerror(errno));
    exit(1);
  }
}

int main(int argc, char **argv) {
  pid_t pid = 100;
  attempt(test_chdir("/test/directory", pid));

  char *cwd = getcwd(0,0);
  attempt(test_chdir(cwd, pid));

  char *cmd = malloc(4096);
  sprintf(cmd, "rm -rf %s.dir", argv[0]);
  printf("system(%s)\n", cmd);
  attempt_errno(system(cmd), cmd);

  sprintf(cmd, "%s.dir", argv[0]);
  printf("mkdir(%s)\n", cmd);
  attempt_errno(mkdir(cmd, 0777), "mkdir");

  attempt(test_chdir(cmd, pid));

  attempt_errno(chdir(cmd), "chdir");
  attempt_errno(mkdir("actual_directory", 0777), "mkdir");
  attempt_errno(symlink("actual_directory", "symlink_directory"), "symlink");

  free(cwd);
  cwd = getcwd(0,0);
  sprintf(cmd, "%s/actual_directory", cwd);
  attempt_errno(symlink(cmd, "abs_symlink_directory"), "symlink");

  attempt(test_chdir("symlink_directory", pid));
  attempt(verify_cwd(cmd, pid));

  attempt(test_chdir(cwd, pid));

  attempt(test_chdir("abs_symlink_directory", pid));
  attempt(verify_cwd(cmd, pid));

  attempt(test_chdir(cwd, pid));
  sprintf(cmd, "%s/actual_directory/subdir", cwd);

  attempt(test_chdir("symlink_directory/subdir", pid));
  attempt(verify_cwd(cmd, pid));

  attempt(test_chdir(cwd, pid));

  attempt(test_chdir("abs_symlink_directory/subdir", pid));
  attempt(verify_cwd(cmd, pid));

  printf("Success!\n");
  return 0;
}
