#!/usr/bin/env bash

set -ev

apt-get update
apt-get install -y git python3 ruby-sass
apt-get install -y build-essential libpopt-dev
apt-get install -y python-markdown graphviz
apt-get install -y clang
# # apt-get install -y scons
apt-get install -y haskell-platform # for ghc to use in a test
# apt-get install -y gnuplot-nox
# apt-get install -y texlive-latex-base texlive-publishers texlive-science
# apt-get install -y feynmf
# apt-get install -y python-matplotlib python-scipy python-markdown
# apt-get install -y python3-matplotlib python3-scipy python3-markdown
# apt-get install -y python-sympy
# apt-get install -y inkscape # for the documentation

