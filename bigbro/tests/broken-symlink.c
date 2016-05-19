#define _BSD_SOURCE
#define _XOPEN_SOURCE 700

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

int main(int argc, char **argv) {
  symlink("silly-me", "tmp/other-link");
  struct stat st;
  lstat("tmp/other-link", &st);
  return 0;
}
