/* bigbro filetracking library
   Copyright (C) 2015 David Roundy

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301 USA */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "bigbro.h"

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
  bigbro(".", &child_pid, 0, args, &read_from_directories,
         &read_from_files, &written_to_files);
  free(args);

  if (read_from_directories)
    for (int i=0; read_from_directories[i]; i++) {
      fprintf(stderr, "l: %s\n", read_from_directories[i]);
    }
  if (read_from_files)
    for (int i=0; read_from_files[i]; i++) {
      fprintf(stderr, "r: %s\n", read_from_files[i]);
    }
  if (written_to_files)
    for (int i=0; written_to_files[i]; i++) {
      fprintf(stderr, "w: %s\n", written_to_files[i]);
    }
  free(read_from_directories);
  free(read_from_files);
  free(written_to_files);
  return 0;
}
