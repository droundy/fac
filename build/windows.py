#!/usr/bin/python3

# bigbro filetracking library
# Copyright (C) 2015,2016 David Roundy
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2 of the
# License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
# 02110-1301 USA

from __future__ import division, print_function

import sys, subprocess, os, shutil, subprocess

cc = 'x86_64-w64-mingw32-gcc'
print('trying', cc)
subprocess.call([cc, '--version'])
cc32 = 'i686-w64-mingw32-gcc'
print('trying', cc32)
subprocess.call([cc32, '--version'])

cflags = ['-std=c99']
objout = lambda fname: '-o'+fname
exeout = objout

def compile(cfile):
    cmd = [cc, '-c', '-O2'] + cflags + [objout(cfile[:-2]+'.obj'), cfile]
    print(' '.join(cmd))
    return subprocess.call(cmd)
def compile32(cfile):
    cmd = [cc32, '-c', '-O2'] + cflags + [objout(cfile[:-2]+'32.obj'), cfile]
    print(' '.join(cmd))
    return subprocess.call(cmd)

shutil.rmtree('bigbro', ignore_errors=True)
subprocess.run('git clone git://github.com/droundy/bigbro', shell=True, check=True)
os.chdir('bigbro')
subprocess.run('python build/windows.py', shell=True, check=True)
os.chdir('..')

print('\nDone building bigbro!\n\n')

cfiles = ['arguments.c', 'build.c', 'clean-all.c', 'environ.c', 'main.c', 'fac.c', 'files.c', 'git.c',
          'lib/iterablehash.c', 'lib/listset.c', 'lib/sha1.c',
          'mkdir.c', 'targets.c']

ofiles = [f[:-1]+'obj' for f in cfiles]

subprocess.run('python generate-version-header.py > version-identifier.h', shell=True, check=True)

# use cc for doing the linking
cmd = ['x86_64-w64-mingw32-gcc', '-g', '-Ibigbro', '-o', 'fac.exe'] + cfiles + ['bigbro/libbigbro-windows.a', '-lpthread']
print(' '.join(cmd))
assert(not subprocess.call(cmd))

shutil.rmtree('bigbro', ignore_errors=True)
