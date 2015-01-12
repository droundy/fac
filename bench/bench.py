#!/usr/bin/python2

import os, hashlib

def make_basedir(n):
    return 'bench/temp/test-%d/' % n

def make_name(i, n):
    path = '.'
    digits = '%d' % i
    while len(digits) < n:
        digits = '0'+digits
    for d in digits:
        path = path+'/number-'+d
    return path[2:]

def make_path(i, n):
    path = 'bench/temp'
    digits = '%d' % i
    while len(digits) < n:
        digits = '0'+digits
    for d in ['test-%d' % n]+['number-'+x for x in digits]:
        try:
            os.mkdir(path)
        except:
            pass
        path = path+'/'+d
    return path

allowed_chars = 'abcdefghijklmnopqrstuvwxyz_ABCDEFGHIJKLMNOPQRSTUVWXYZ'

def hashid(n):
    m = hashlib.sha1()
    m.update(str(n))
    name = ''
    for i in m.digest()[:16]:
        name += allowed_chars[ord(i) % len(allowed_chars)]
    return name

def hash_integer(n):
    m = hashlib.sha1()
    m.update(str(n))
    name = 0
    for i in m.digest()[:8]:
        name = name*256 + ord(i)
    return name

os.system('rm -rf bench/temp')

def create_bench(n):
    make_path(0, n)
    bilgef = open(make_basedir(n)+'top.bilge', 'w')
    tupf = open(make_basedir(n)+'Tupfile', 'w')
    makef = open(make_basedir(n)+'Makefile', 'w')
    open(make_basedir(n)+'Tupfile.ini', 'w')
    makef.write('all:')
    for i in range(10**n):
        makef.write(' %s.o' % make_name(i, n))
    makef.write("""

clean:
\trm -f *.o */*.o */*/*.o */*/*/*.o */*/*/*/*.o
""")
    for i in range(10**n):
        base = make_path(i, n)
        cname = make_name(i, n) + '.c'
        hname = make_name(i, n) + '.h'
        includes = ''
        funcs = ''
        include_deps = ''
        for xx in [i+ii for ii in range(10)]:
            includes += '#include "%s.h"\n' % make_name(hash_integer(xx) % 10**n, n)
            funcs += '    %s();\n' % hashid(hash_integer(xx) % 10**n)
            include_deps += ' %s.h' % make_name(hash_integer(xx) % 10**n, n)
        makef.write("""
# %d
%s.o: %s.c %s
\tgcc -I. -O2 -c -o %s.o %s.c
""" % (i, make_name(i, n), make_name(i, n), include_deps,
       make_name(i, n), make_name(i, n)))
        tupf.write("""
# %d
: %s.c |> gcc -I. -O2 -c -o %%o %%f |> %s.o
""" % (i, make_name(i, n), make_name(i, n)))
        bilgef.write("""
# %d
| gcc -I. -O2 -c -o %s.o %s.c
> %s.o
""" % (i, make_name(i, n), make_name(i, n), make_name(i, n)))
        f = open(base+'.c', 'w')
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
        f = open(base+'.h', 'w')
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

for nnn in range(1, 5):
    create_bench(nnn)
