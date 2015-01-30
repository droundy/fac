#define _XOPEN_SOURCE 700

#include "bilge.h"

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <assert.h>
#include <stdlib.h>

void add_git_files(struct all_targets *all) {
  const char *templ = "/tmp/bilge-XXXXXX";
  char *namebuf = malloc(strlen(templ)+1);
  strcpy(namebuf, templ);
  int out = mkstemp(namebuf);
  unlink(namebuf);
  free(namebuf);

  pid_t new_pid = fork();
  if (new_pid == 0) {
    char **args = malloc(3*sizeof(char *));
    close(1); dup(out);
    close(2);
    open("/dev/null", O_WRONLY);
    args[0] = "git";
    args[1] = "ls-files";
    args[2] = 0;
    execvp("git", args);
    exit(0);
  }
  int status = 0;
  if (waitpid(new_pid, &status, 0) != new_pid) {
    printf("Unable to exec git ls-files\n");
    return; // fixme should exit
  }
  if (WEXITSTATUS(status)) {
    printf("Unable to run git ls-files successfully %d\n", WEXITSTATUS(status));
    //    return 0;
  }
  off_t stdoutlen = lseek(out, 0, SEEK_END);
  lseek(out, 0, SEEK_SET);
  char *buf = malloc(stdoutlen);
  if (read(out, buf, stdoutlen) != stdoutlen) {
    printf("Error reading output of git ls-files\n");
    free(buf);
    return; // fixme should exit
  }

  int last_start = 0;
  for (int i=0;i<stdoutlen;i++) {
    if (buf[i] == '\n') {
      buf[i] = 0;
      char *path = absolute_path(root, buf + last_start);
      struct target *t = create_target(all, path);
      free(path);
      assert(t);
      t->is_in_git = true;
      t->exists = true;
      last_start = i+1;
    }
  }
  free(buf);
}
