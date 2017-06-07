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

def open_and_gitadd(fname):
    f = open(fname, 'w')
    assert(not os.system('git add '+fname))
    return f

def create_bench(n):
    sconsf = open_and_gitadd('SConstruct')
    facf = open_and_gitadd('top.fac')
    open_and_gitadd('Tupfile.ini')
    sconsf.write("""
env = Environment()
""")
    facf.write("""
| cat %s.txt > final.txt
> final.txt
< %s.txt

""" % (hashid(n), hashid(n)))
    f = open_and_gitadd('%s.txt' % hashid(0))
    f.write("Hello world\n")
    for i in range(1,n+1):
        sconsf.write("""
env.Command('%s.txt', '%s.txt', 'cat $SOURCE > $TARGET')
""" % (hashid(i), hashid(i-1)))
        facf.write("""
| cat %s.txt > %s.txt
< %s.txt
> %s.txt
""" % (hashid(i-1), hashid(i), hashid(i-1), hashid(i)))

verbs = ['building', 'rebuilding', 'doing-nothing']

def prepare():
    return {'rebuilding': 'sleep 1 && echo silly > %s.txt' % hashid(0)}
