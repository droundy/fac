FROM debian:stretch-slim

# I use separate RUN statements here hoping to be able to make changes
#  without invalidating the entire cache.

RUN apt-get -y update && apt-get -y upgrade && apt-get -y install apt-utils
RUN apt-get -y install texlive-latex-base ghc make valgrind wamerican-small gcc python3 git python3-markdown gcovr libc6-dev-i386 help2man checkinstall screen ruby-sass python3-matplotlib

RUN git clone git://git.kernel.org/pub/scm/devel/sparse/chrisl/sparse.git /root/sparse
RUN cd /root/sparse && make && cp sparse /usr/bin/

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
