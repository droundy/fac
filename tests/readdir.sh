#!/bin/sh

set -ev

cd tests
rm -rf readdir
mkdir readdir
cd readdir
cat > top.bilge <<EOF
| echo *.message > messages
> messages
EOF

touch foo.message
../../bilge

grep foo.message messages

# touch bar.message
# ../../bilge

# grep foo.message messages
# grep bar.message messages

exit 0
