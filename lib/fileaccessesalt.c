#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "bigbrotheralt.h"

int main(int argc, char **argv) {
  if (argc < 2) {
    fprintf(stderr, "Usage: %s prog args\n", argv[0]);
    exit(1);
  }

  hashset written_to_files;
  hashset read_from_files;
  hashset read_from_directories;
  hashset deleted_files;
  initialize_hashset(&written_to_files);
  initialize_hashset(&read_from_files);
  initialize_hashset(&read_from_directories);
  initialize_hashset(&deleted_files);

  char **args = (char **)malloc(argc*sizeof(char*));
  memcpy(args, argv+1, (argc-1) * sizeof(char*));
  args[argc-1] = NULL;
  pid_t child_pid;
  bigbrotheralt_process(".", &child_pid, 0, args, &read_from_directories,
                        &read_from_files, &written_to_files, &deleted_files);
  free(args);

  for (struct set_entry *e = (struct set_entry *)read_from_directories.first;
       e; e = (struct set_entry *)e->e.next) {
    char *path = e->key;
    fprintf(stderr, "l: %s\n", path);
  }
  for (struct set_entry *e = (struct set_entry *)read_from_files.first;
       e; e = (struct set_entry *)e->e.next) {
    char *path = e->key;
    fprintf(stderr, "r: %s\n", path);
  }
  for (struct set_entry *e = (struct set_entry *)written_to_files.first;
       e; e = (struct set_entry *)e->e.next) {
    char *path = e->key;
    fprintf(stderr, "w: %s\n", path);
  }
  for (struct set_entry *e = (struct set_entry *)deleted_files.first;
       e; e = (struct set_entry *)e->e.next) {
    char *path = e->key;
    fprintf(stderr, "d: %s\n", path);
  }
  free_hashset(&read_from_directories);
  free_hashset(&read_from_files);
  free_hashset(&written_to_files);
  free_hashset(&deleted_files);
  return 0;
}
