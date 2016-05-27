#define _POSIX_C_SOURCE 200809L

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int main() {
  int tmpd = open("tmp", O_RDONLY | O_DIRECTORY);
  openat(tmpd, "openat", O_WRONLY | O_CREAT | O_EXCL, 0666);
  return 0;
}
