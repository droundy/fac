#!/bin/sh

set -ev

FAC=`pwd`/fac

chmod -R +xr /tmp/$0.dir || echo no such dir
rm -rf /tmp/$0.dir
mkdir -p /tmp/$0.dir
cd /tmp/$0.dir

cat > my.fac <<EOF
| cat foo bar > out || true

| mkdir -p directory && echo hello > directory/hello
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

pwd
chmod a-x ..

if cd ..; then
    echo we have some funky broken filesystem here
else

    if $FAC &> fac.err; then
        cat fac.err
        echo should have failed error identifying
        exit 1
    fi

    cat fac.err
    # the following is perhaps a confusing error, but it's a confusing situation
    grep 'Unable to run git rev-parse successfully' fac.err

    chmod +x ..

    cd ..
fi

$FAC -c

if ls directory; then
    echo directory should have been deleted
    exit 1
fi

chmod a-r ..

$FAC --dry

$FAC

chmod a+r ..
ls -l
ls directory

$FAC -c

if ls directory; then
    echo directory should have been deleted
    exit 1
fi
echo all good

exit 0
