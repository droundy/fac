#!/bin/sh

set -ev

if ! which ghc; then
    echo there is no ghc
    exit 137
fi

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

cat > Foo.hs <<EOF
module Foo where

foo = "hello "
EOF

cat > Bar.hs <<EOF
module Bar where

bar = "world"
EOF

cat > main.hs <<EOF
module Main where

import Foo
import Bar

main = putStrLn $ foo ++ bar
EOF

cat > top.fac <<EOF
| ghc -c Foo.hs
> Foo.o

| ghc -c Bar.hs
> Bar.o

| ghc --make -o main main.hs
> main
< Foo.o
< Bar.o

EOF

git init
git add top.fac main.hs Foo.hs Bar.hs

${FAC:-../../fac}

exit 0
