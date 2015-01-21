#!/usr/bin/python3

import os, hashlib, time, numpy, sys, datetime

name = 'cat'

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
    m.update(str(n))
    name = 0
    for i in m.digest()[:8]:
        name = name*256 + ord(i)
    return name

def create_bench(n):
    sconsf = open('SConstruct', 'w')
    bilgef = open('top.bilge', 'w')
    tupf = open('Tupfile', 'w')
    makef = open('Makefile', 'w')
    open('Tupfile.ini', 'w')
    sconsf.write("""
env = Environment()
""")
    makef.write("""
final.txt: %s.txt
\tcat $< > $@

""" % hashid(n))
    bilgef.write("""
| cat %s.txt > final.txt
> final.txt
< %s.txt

""" % (hashid(n), hashid(n)))
    f = open('%s.txt' % hashid(0), 'w')
    f.write("Hello world\n")
    for i in range(1,n+1):
        makef.write("""
# %d
%s.txt: %s.txt
\tcat $< > $@
""" % (i, hashid(i), hashid(i-1)))
        tupf.write("""
# %d
: %s.txt |> cat %%f > %%o |> %s.txt
""" % (i, hashid(i-1), hashid(i)))
        bilgef.write("""
# %d
| cat %s.txt > %s.txt
> %s.txt
< %s.txt
""" % (i, hashid(i-1), hashid(i), hashid(i), hashid(i-1)))
        sconsf.write("""
env.Command('%s.txt', '%s.txt', 'cat $SOURCE > $TARGET')
""" % (hashid(i), hashid(i-1)))

verbs = ['building', 'rebuilding', 'doing-nothing']

def prepare():
    return {'rebuilding': 'sleep 1 && touch %s.txt' % hashid(0)}
