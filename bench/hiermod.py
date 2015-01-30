#!/usr/bin/python3

import os, hashlib, time, numpy, sys, datetime

name = 'hierarchy'

def make_name(i):
    path = '.'
    digits = '%d' % i
    for d in digits:
        path = path+'/number-'+d
    return path[2:]+('_%d' % i)

allowed_chars = 'abcdefghijklmnopqrstuvwxyz_ABCDEFGHIJKLMNOPQRSTUVWXYZ'

def hashid(n):
    m = hashlib.sha1()
    m.update(str(n).encode('utf-8'))
    name = ''
    for i in m.digest()[:24]:
        name += allowed_chars[i % len(allowed_chars)]
    return name+('_%d' % n)

def hash_integer(n):
    m = hashlib.sha1()
    m.update(str(n).encode('utf-8'))
    name = 0
    for i in m.digest()[:8]:
        name = name*256 + i
    return abs(name)

def open_and_gitadd(fname):
    f = open(fname, 'w')
    assert(not os.system('git add '+fname))
    return f

def create_bench(N):
    sconsf = open_and_gitadd('SConstruct')
    loonf = open_and_gitadd('top.loon')
    open_and_gitadd('Tupfile.ini')
    sconsf.write("""
env = Environment(CPPPATH=['.'])
""")
    for i in range(N):
        if i > 9:
            os.makedirs(os.path.dirname(make_name(i)), exist_ok=True)
        cname = make_name(i) + '.c'
        hname = make_name(i) + '.h'
        includes = ''
        funcs = ''
        for xx in [i+ii for ii in range(10)]:
            nhere = hash_integer(xx) % N
            includes += '#include "%s.h"\n' % make_name(nhere)
            funcs += '    %s();\n' % hashid(nhere)
        loonf.write("""
# %d
| gcc -I. -O2 -c -o %s.o %s.c
> %s.o
""" % (i, make_name(i), make_name(i), make_name(i)))
        sconsf.write("""
env.Object('%s.c')
""" % make_name(i))
        f = open_and_gitadd(make_name(i)+'.c')
        f.write('\n')
        f.write("""/* c file %s */

%s#include "%s"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string.h>
#include <limits.h>
#include <complex.h>
#include <time.h>

void delete_from_%s(list%s **list, const char *path) {
  while (*list != NULL) {
    if (strcmp((*list)->path, path) == 0) {
      list%s *to_be_deleted = *list;
      *list = (*list)->next;
      free(to_be_deleted->path);
      free(to_be_deleted);
      return;
    }
    list = &((*list)->next);
  }
}

int is_in_%s(const list%s *ptr, const char *path) {
  while (ptr != NULL) {
    if (strcmp(ptr->path, path) == 0) {
      return 1;
    }
    ptr = ptr->next;
  }
  return 0;
}

void insert_to_%s(list%s **list, const char *path) {
  delete_from_%s(list, path);
  list%s *newhead = (list%s *)malloc(sizeof(list%s));
  newhead->next = *list;
  newhead->path = malloc(strlen(path)+1);
  strcpy(newhead->path, path);

  *list = newhead;
}

void free_%s(list%s *list) {
  while (list != NULL) {
    list%s *d = list;
    list = list->next;
    free(d->path);
    free(d);
  }
}

static void stupid_this(char *s) {
  while (*s) {
    while (*s < 'a') {
      *s += 13;
    }
    while (*s > 'z') {
      *s -= 13;
    }
    printf("%%c", *s);
    s++;
  }
  printf("\\n");
}

void %s() {
    stupid_this("Hello world %s!\\n");
%s}
""" % (cname, includes, hname, hashid(i), hashid(i),
       hashid(i), hashid(i), hashid(i), hashid(i), hashid(i),
       hashid(i), hashid(i), hashid(i), hashid(i), hashid(i),
       hashid(i), hashid(i), hashid(i), hashid(i),
       funcs))
        f.close()
        f = open_and_gitadd(make_name(i)+'.h')
        f.write("""/* header %s */
#ifndef %s_H
#define %s_H

typedef struct list%s {
  char *path;
  struct list%s *next;
} list%s;

void insert_to_%s(list%s **list, const char *path);
void delete_from_%s(list%s **list, const char *path);
int is_in_%s(const list%s *list, const char *path);

void free_%s(list%s *list);

void %s();

#endif
""" % (hname,
       hashid(i), hashid(i), hashid(i), hashid(i), hashid(i),
       hashid(i), hashid(i), hashid(i), hashid(i), hashid(i),
       hashid(i), hashid(i), hashid(i), hashid(i)))
        f.close()
    return N

verbs = ['building', 'rebuilding', 'touching-header', 'touching-c', 'doing-nothing']

def prepare():
    return {'rebuilding': r'sleep 1 && find . -name "*.c" -exec touch \{\} \;',
            'touching-header': 'echo >> number-0.h',
            'touching-c': 'echo >> number-0.c'}
