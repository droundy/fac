#define _BSD_SOURCE
#define _XOPEN_SOURCE 700

#include <fcntl.h>
#include <unistd.h>

int main(int argc, char **argv) {
  return symlinkat("foobar", AT_FDCWD, "tmp/new-symlink") + symlink("foo", "tmp/other-link");
}
