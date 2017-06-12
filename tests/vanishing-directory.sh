#!/bin/sh

set -ev

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

git init

cat > build.fac <<EOF
| mkdir foo-directory && rmdir foo-directory && echo bar > bar
EOF

git add build.fac

${FAC:-../../fac}

grep bar bar

cat build.fac.tum

if grep '> foo-directory' build.fac.tum; then
    echo we did not actually create foo-directory
    exit 1
fi

exit 0
