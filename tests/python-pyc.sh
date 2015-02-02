#!/bin/sh

set -ev

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

cat > my_nice_test.py <<EOF
greeting = 'hello world'
EOF

cat > main.py <<EOF
import my_nice_test

print(my_nice_test.greeting)
EOF

../../lib/fileaccesses python main.py 2> err

grep my_nice_test err

egrep my_nice_test.py\$ err
grep my_nice_test.pyc err

../../lib/fileaccesses python main.py 2> err

egrep my_nice_test.py\$ err
grep my_nice_test.pyc err

exit 0
