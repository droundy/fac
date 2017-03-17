FROM debian:stretch-slim

# This docker image provides all the prerequisites for building and
# testing fac.

# need curl for codecov uploading

RUN apt-get -y update && apt-get -y upgrade && apt-get -y install apt-utils && apt-get -y install texlive-latex-base ghc make valgrind wamerican-small sparse gcc python3 git python3-markdown gcovr libc6-dev-i386 help2man checkinstall screen curl

RUN git clone git://git.kernel.org/pub/scm/devel/sparse/chrisl/sparse.git /root/sparse && cd /root/sparse && make && cp sparse /usr/bin/

COPY test.sh test.sh
