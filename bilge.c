#define _BSD_SOURCE

#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <error.h>
#include <errno.h>
#include <string.h>

#include "bilge.h"

void go_to_bilge_top() {
  while (1) {
    DIR *dir = opendir(".");
    char *dirname = getcwd(0,0);
    struct dirent entry, *result;

    if (strcmp(dirname, "/") == 0)
      error(1, errno, "could not locate top.bilge!");

    if (!dir) error(1, errno, "unable to opendir %s", dirname);

    if (readdir_r(dir, &entry, &result))
      error(1, errno, "error reading from %s", dirname);
    while (result) {
      if (!strcmp("top.bilge", entry.d_name)) {
        closedir(dir);
        free(dirname);
        return;
      }

      if (readdir_r(dir, &entry, &result))
        error(1, errno, "error reading from %s", dirname);
    }
    closedir(dir);

    if (chdir("..")) error(1, errno, "unable to chdir(..) from %s", dirname);
    free(dirname);
  }
}

void find_bilge_files(struct all_targets **all, const char *dirname, int level) {
  DIR *dir = opendir(dirname);
  struct dirent entry, *result;

  //read_bilge_file(all, "top.bilge");

  if (!dir) error(1, errno, "unable to opendir %s", dirname);

  if (readdir_r(dir, &entry, &result))
    error(1, errno, "error reading from %s", dirname);
  while (result) {
    if (strcmp(".", entry.d_name) &&
        strcmp(".git", entry.d_name) &&
        strcmp("..", entry.d_name)) {
      //printf("%*sentry: %s\n", level*2, "", entry.d_name);
      if (entry.d_type == DT_DIR) {
        int len = strlen(dirname)+strlen(entry.d_name)+2;
        char *subdir = malloc(len);
        if (snprintf(subdir, len, "%s/%s", dirname, entry.d_name) >= len)
          error(1, errno, "I make bug in subdir len");
        find_bilge_files(all, subdir, level+1);
        free(subdir);
      } else {
        const int dlen = strlen(entry.d_name);
        if (dlen >= 6 && !strcmp(entry.d_name+dlen-6, ".bilge")) {
          //printf("%*s - it is a .bilge file!\n", 2*level, "");
          int len = strlen(dirname)+strlen(entry.d_name)+2;
          char *bilgefile = malloc(len);
          if (snprintf(bilgefile, len, "%s/%s", dirname, entry.d_name) >= len)
            error(1, errno, "I make bug in bilgefile len");
          read_bilge_file(all, bilgefile);
          free(bilgefile);
        }
      }
    }

    if (readdir_r(dir, &entry, &result))
      error(1, errno, "error reading from %s", dirname);
  }
  closedir(dir);

  return;
}

static struct all_targets *all = 0;

int main(int argc, char **argv) {
  go_to_bilge_top();

  find_bilge_files(&all, ".", 0);

  /* printf("NOW I AM PRINTING!!!\n"); */
  /* print_bilge_file(all); */

  /* printf("NOW I AM BUILDING!!!\n"); */
  build_all(&all);

  return 0;
}
