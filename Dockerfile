FROM debian:stretch-slim

# I use separate RUN statements here hoping to be able to make changes
#  without invalidating the entire cache.

RUN apt-get -y update
RUN apt-get -y upgrade
RUN apt-get -y install apt-utils
RUN apt-get -y install texlive
RUN apt-get -y install ghc
RUN apt-get -y install make valgrind wamerican-small sparse
RUN apt-get -y install gcc python3 git python3-markdown
RUN apt-get -y install lcov
RUN apt-get -y install gcovr
RUN apt-get -y install libc6-dev-i386

RUN useradd developer
RUN mkdir /home/developer

# COPY . /home/developer/git-source-repo
# RUN su developer -c 'git clone /home/developer/git-source-repo /home/developer/fac'
COPY . /home/developer/fac
RUN chown -R developer:developer /home/developer

WORKDIR /home/developer/fac
USER developer

RUN git config --global user.email "testing-on-gitlab-ci@example.com"
RUN git config --global user.name "CI Builder"

# execute this with:
# docker build -t fac . && docker run --security-opt seccomp:./docker-security.json fac python3 run-tests.py
