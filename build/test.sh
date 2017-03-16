#!/bin/bash

set -ev

useradd developer
mkdir /home/developer
chown -R developer:developer /home/developer
chown -R developer:developer fac

cd fac
su developer

git config --global user.email "testing-on-gitlab-ci@example.com"
git config --global user.name "CI Builder"

python3 run-tests.py -v'

# execute this with:
# docker build -t facio/build .
# docker run --security-opt seccomp:../docker-security.json facio/build bash -c "git clone git://github.com/droundy/fac && bash fac/build/test.sh"
