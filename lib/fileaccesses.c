#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "bigbrother.h"

int main(int argc, char **argv) {
  if (argc < 2) {
    fprintf(stderr, "Usage: %s prog args\n", argv[0]);
    exit(1);
  }

  arrayset written_to_files;
  arrayset read_from_files;
  arrayset read_from_directories;
  arrayset deleted_files;
  initialize_arrayset(&written_to_files);
  initialize_arrayset(&read_from_files);
  initialize_arrayset(&read_from_directories);
  initialize_arrayset(&deleted_files);

  char **args = (char **)malloc(argc*sizeof(char*));
  memcpy(args, argv+1, (argc-1) * sizeof(char*));
  args[argc-1] = NULL;
  pid_t child;
  bigbrother_process_arrayset(".", args, &child, &read_from_directories,
                              &read_from_files, &written_to_files, &deleted_files);
  free(args);

  for (char *path = start_iterating(&read_from_directories);path;path = iterate(&read_from_directories))
    fprintf(stderr, "l: %s\n", path);
  for (char *path = start_iterating(&read_from_files);path;path = iterate(&read_from_files))
    fprintf(stderr, "r: %s\n", path);
  for (char *path = start_iterating(&written_to_files);path;path = iterate(&written_to_files))
    fprintf(stderr, "w: %s\n", path);
  for (char *path = start_iterating(&deleted_files);path;path = iterate(&deleted_files))
    fprintf(stderr, "d: %s\n", path);
  return 0;
}
