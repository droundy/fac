#!/usr/bin/python3

import sys, os, re, subprocess

if sys.version_info < (3,5):
    print('Please run this script with python 3.5 or newer:', sys.version)
    exit(137)

runre = re.compile(r'\[run\]: # \((.+)\)')
shellre = re.compile(r'^    \$ (.+)')
filere = re.compile(r'##### (.+)')
verbre = re.compile(r'^    (.*)')
time_remaining_re = re.compile(r'^Build time remaining: ')

with open(sys.argv[1]) as f:
    for line in f:
        isfile = filere.findall(line)
        isshell = shellre.findall(line)
        if len(isfile) > 0:
            with open(isfile[0], 'w') as newf:
                for line in f:
                    isverb = verbre.findall(line)
                    if len(isverb) == 0:
                        break
                    newf.write(isverb[0])
                    newf.write('\n')
                    print(isfile[0], ':', isverb[0])
        elif len(isshell) > 0:
            print('shell :', isshell[0])
            tocheck = True
            if isshell[0][-len('# fails'):] == '# fails':
                tocheck = False
                print('SHOULD FAIL!')
                isshell[0] = isshell[0][:-len('# fails')]
            ret = subprocess.run(isshell, shell=True,
                                 stderr=subprocess.STDOUT,
                                 check=tocheck,
                                 stdout=subprocess.PIPE)
            if not tocheck and ret.returncode == 0:
                print("DID NOT FAIL!!!")
                exit(1)
            print('output:', ret.stdout)
            output = ret.stdout
            for outline in output.decode('utf-8').split('\n'):
                # The time_remaining_re bit is needed to skip the
                # "Build time remaining:" lines that get printed every
                # once in a while.  These are irregular, which is why
                # we need to do this.
                if len(outline)>0 and not time_remaining_re.match(outline):
                    print('output:', outline)
                    expectedline = f.readline()
                    if len(verbre.findall(expectedline)) == 0:
                        print('unexpected output from:', isshell[0])
                        print('output is', outline)
                        exit(1)
                    if expectedline in ['    ...', '    ...\n']:
                        print('I expected random output.')
                        break
                    expected = verbre.findall(expectedline)[0]
                    expected = expected.replace('.', r'\.')
                    expected = expected.replace('*', r'\*')
                    expected = expected.replace(r'\.\.\.', '.*')
                    expected = expected.replace('[', r'\[')
                    expected = expected.replace(']', r'\]')
                    expected = expected.replace('(', r'\(')
                    expected = expected.replace(')', r'\)')
                    if not re.compile(expected).match(outline):
                        print('I expected:', expected)
                        print('but instead I got:', outline)
                        exit(1)
        else:
            print('input', line.strip())
