#ifndef NEW_BUILD_H
#define NEW_BUILD_H

#include "bilge.h"

void mark_bilgefiles(struct all_targets *all);
void mark_all(struct all_targets *all);

void build_marked(struct all_targets *all, const char *root_);
void check_for_impossibilities(struct all_targets *all, const char *_root);

#endif
