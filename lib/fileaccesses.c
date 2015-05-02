#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "bigbrother.h"

int main(int argc, char **argv) {
  if (argc < 2) {
    fprintf(stderr, "Usage: %s prog args\n", argv[0]);
    exit(1);
  }

  char **written_to_files = 0;
  char **read_from_files = 0;
  char **read_from_directories = 0;

  char **args = (char **)malloc(argc*sizeof(char*));
  memcpy(args, argv+1, (argc-1) * sizeof(char*));
  args[argc-1] = NULL;
  pid_t child_pid;
  bigbrother_process(".", &child_pid, 0, args, &read_from_directories,
                     &read_from_files, &written_to_files);
  free(args);

  for (int i=0; read_from_directories[i]; i++) {
    fprintf(stderr, "l: %s\n", read_from_directories[i]);
  }
  for (int i=0; read_from_files[i]; i++) {
    fprintf(stderr, "l: %s\n", read_from_files[i]);
  }
  for (int i=0; written_to_files[i]; i++) {
    fprintf(stderr, "l: %s\n", written_to_files[i]);
  }
  free(read_from_directories);
  free(read_from_files);
  free(written_to_files);
  return 0;
}
