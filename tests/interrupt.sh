#!/bin/sh

set -ev

echo $0

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

cat > top.fac <<EOF
| cp input output && sleep 5

| cp input output1 && sleep 5

| sleep 5 && cp input output2
EOF

echo input > input

git init
git add top.fac input

${FAC:-../../fac} -v &
ID=$!

sleep 1

kill -s SIGINT $ID

sleep 2
echo done killing?

ls -trlh

if grep input output; then
    echo this should have been deleted when code was interrupted
    exit 1
fi

${FAC:-../../fac}

exit 0
