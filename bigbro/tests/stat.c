#include <sys/stat.h>

int main() {
  struct stat st;
  stat("tmp/foo", &st);
  return 0;
}
