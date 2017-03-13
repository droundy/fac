#!/bin/sh

set -ev

FAC=`pwd`/fac

chmod -R +xr /tmp/$0.dir || echo no such dir
rm -rf /tmp/$0.dir
mkdir -p /tmp/$0.dir
cd /tmp/$0.dir

cat > my.fac <<EOF
| cat foo bar > out || true

EOF

if $FAC &> fac.err; then
    cat fac.err
    echo should have failed here not git
    exit 1
fi

cat fac.err
grep 'Error identifying git top' fac.err

git init

if $FAC &> fac.err; then
    cat fac.err
    echo should have failed here no .fac
    exit 1
fi

cat fac.err
grep 'Please add a .fac file' fac.err

git add my.fac

$FAC

mkdir subdir
cd subdir

chmod a-x ..

if $FAC &> fac.err; then
    cat fac.err
    echo should have failed error identifying
    exit 1
fi

cat fac.err
# the following is perhaps a confusing error, but it's a confusing situation
grep 'Unable to run git rev-parse successfully' fac.err

chmod +x ..

$FAC -c

chmod a-r ..

$FAC

exit 0
