#!/bin/bash

set -ev

useradd developer
mkdir /home/developer
chown -R developer:developer /home/developer

cd /home/developer
su - developer

git config --global user.email "testing-on-gitlab-ci@example.com"
git config --global user.name "CI Builder"

git clone git://github.com/droundy/fac
cd fac

sh build/linux.sh
python3 run-tests.py -v

# execute this with:
# docker build -t facio/build .
# docker run --security-opt seccomp:../docker-security.json facio/build bash test.sh
