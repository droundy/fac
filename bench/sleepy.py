#!/usr/bin/python3

import os, hashlib, time, numpy, sys

name = 'sleepy'

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
    longsleep = max((n-9)//3, 0)
    shortsleep = 1
    if longsleep == 0:
        shortsleep = 0
    final_indices = range(1,n+1,3)
    long_indices = [final_indices[0],
                    final_indices[(n//8) % len(final_indices)],
                    final_indices[(n//4) % len(final_indices)]]
    finalfiles = [hashid(i)+'.txt' for i in final_indices]

    sconsf = open('SConstruct', 'w')
    bilgef = open('top.bilge', 'w')
    open('Tupfile.ini', 'w')
    sconsf.write("""
env = Environment()

env.Command('final.txt', ['%s'], 'sleep 1 && cat $SOURCE > $TARGET')
""" % ("', '".join(finalfiles)))
    bilgef.write("""
| cat %s > final.txt
> final.txt
< %s

""" % (' '.join(finalfiles), "\n< ".join(finalfiles)))
    for i in final_indices:
        sleepiness = shortsleep
        if i in long_indices:
            sleepiness = longsleep
        bilgef.write("""
| sleep %d && cat %s.txt > %s.txt
< %s.txt
> %s.txt
| sleep %d && cat %s.txt > %s.txt
< %s.txt
> %s.txt
| sleep %d && echo %s > %s.txt
> %s.txt
""" % (sleepiness, hashid(i+1), hashid(i), hashid(i+1), hashid(i),
       sleepiness, hashid(i+2), hashid(i+1), hashid(i+2), hashid(i+1),
       sleepiness, hashid(i), hashid(i+2), hashid(i+2)))
        sconsf.write("""
env.Command('%s.txt', '%s.txt', 'sleep %d && cat $SOURCE > $TARGET')
env.Command('%s.txt', '%s.txt', 'sleep %d && cat $SOURCE > $TARGET')
env.Command('%s.txt', [], 'sleep %d && echo %s > $TARGET')
""" % (hashid(i), hashid(i+1), sleepiness,
       hashid(i+1), hashid(i+2), sleepiness,
       hashid(i+2), sleepiness, hashid(i)))

verbs = ['building', 'rebuilding']

def prepare():
    return {'rebuilding': 'rm -f *.txt'}
