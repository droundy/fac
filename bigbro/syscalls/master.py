#!/usr/bin/python2

import re, os

call_re = re.compile(r'(\d+)\s+\S+[^\{]+\s+\{ \S+ (\S+)\s*\(')
obsol_re = re.compile(r'(\d+)\s+\S+\s+(OBSOL|UNIMPL|NODEF.NOTSTATIC)\s+(\S+)')

def parse(master):
    linenum = 0
    syscallnames = []
    for line in master.split('\n'):
        linenum += 1
        #print 'line %4d' % linenum, line
        m = call_re.match(line)
        results = None
        if m != None:
            results = m.groups()
        else:
            m = obsol_re.match(line)
            if m != None:
                #print 'obsolete', m.groups()
                results = (m.groups()[0], m.groups()[2])
        if results != None:
            num, name = results
            #print num, name, len(syscallnames)
            assert(int(num) == len(syscallnames))
            syscallnames.append(name)
    return syscallnames

if __name__ == "__main__":
    import sys
    fname = 'syscalls/freebsd/syscalls.master'
    if len(sys.argv) == 2:
        fname = sys.argv[1]

    with open(fname) as f:
        print(parse(f.read()))
