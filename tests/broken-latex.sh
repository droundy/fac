#!/bin/sh

set -ev

echo $0

# This test ensures that we can count builds properly to figure out if
# we have built everything correctly.

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

mkdir subdir

cat > top.fac <<EOF
| pdflatex foo.tex

EOF

cat > foo.tex <<EOF
\documentclass{article}

\begin{document}
EOF

git init
git add top.fac foo.tex

if ../../fac; then
  echo it was a bad document and should have failed
  exit 1
fi

exit 0
