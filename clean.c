#include "loon.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static const char *pretty_path(const char *path) {
  int len = strlen(root);
  if (path[len] == '/' && !memcmp(path, root, len)) {
    return path + len + 1;
  }
  return path;
}

void clean_all(struct all_targets *all) {
  for (struct target *t = (struct target *)all->t.first; t; t = (struct target *)t->e.next) {
    t->status = unknown;
    int len = strlen(t->path);
    if (len >= 6 && !strcmp(t->path+len-6, ".bilge")) {
      char *donef = malloc(len + 20);
      strcpy(donef, t->path);
      strcat(donef, ".done");
      printf("rm %s\n", donef);
      unlink(donef);
      free(donef);
    }
  }
  for (struct rule *r = (struct rule *)all->r.first; r; r = (struct rule *)r->e.next) {
    for (int i=0;i<r->num_outputs;i++) {
      if (r->outputs[i]->status == unknown) {
        printf("rm %s\n", pretty_path(r->outputs[i]->path));
        unlink(r->outputs[i]->path);
      }
    }
  }
}
