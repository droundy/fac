#!/usr/bin/python3

import sys, os, re, subprocess

runre = re.compile(r'\[run\]: # \((.+)\)')
shellre = re.compile(r'^    \$ (.+)')
filere = re.compile(r'##### (.+)')
verbre = re.compile(r'^    (.*)')

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
            output = subprocess.run(isshell, shell=True,
                                    stderr=subprocess.STDOUT,
                                    check=tocheck,
                                    stdout=subprocess.PIPE).stdout
            for outline in output.decode('utf-8').split('\n'):
                if len(outline)>0:
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
