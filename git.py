#!/usr/bin/python3

"""

This is a small script that enables conveniently running git with a
modified gnupg environment.  The purpose is to enable users to
conveniently verify that commits and tags have been signed by actual
authorized developers.  For more information, read

http://physics.oregonstate.edu/~roundyd/fac/signatures.html

 """

import sys, subprocess, os

if not os.path.exists('git.py'):
    print('error: must call this program from its own directory')
    exit(1)

wd = os.getcwd()

env = os.environ
env['GNUPGHOME'] = wd+'/.gnupg'

args = sys.argv[1:]
if args[0] == 'pull':
    args = [args[0], '--verify-signatures'] + args[1:]

exit(subprocess.call(['git'] + args, env=env))
