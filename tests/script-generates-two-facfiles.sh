#!/bin/sh

set -ev

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

git init

cat > top.fac <<EOF
| sh fac.sh
> 1.fac
> 2.fac
EOF
git add top.fac

cat > fac.sh <<EOF
#!/bin/sh

cat > 1.fac <<EOME
| echo foo > foo
EOME

cat > 2.fac <<EOME
| echo bar > bar
EOME
EOF
git add fac.sh

${FAC:-../../fac} -v

grep foo foo

grep bar bar

cat > fac.sh <<EOF
#!/bin/sh

cat > 1.fac <<EOME
| echo one > foo
EOME

cat > 2.fac <<EOME
| echo two > bar
EOME

EOF

cat top.fac.tum

${FAC:-../../fac} -v

grep one foo
grep two bar

echo it all worked

exit 0
