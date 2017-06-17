FROM debian:stretch-slim

# This docker image provides all the prerequisites for building and
# testing fac.

# need curl for codecov uploading

RUN apt-get -y update && apt-get -y upgrade && apt-get -y install apt-utils && apt-get -y install texlive-latex-base ghc make valgrind wamerican-small gcc python3 git python3-markdown gcovr libc6-dev-i386 help2man checkinstall screen curl ruby-sass python3-matplotlib gcc-mingw-w64 ninja-build

RUN curl https://sh.rustup.rs -sSf | sh -s -- --default-toolchain stable -y

RUN git clone git://git.kernel.org/pub/scm/devel/sparse/chrisl/sparse.git /root/sparse && cd /root/sparse && make && cp sparse /usr/bin/

COPY test.sh test.sh

COPY cargo-config /.cargo/config

ENV PATH=/root/.cargo/bin:$PATH

RUN rustup target add x86_64-pc-windows-gnu

# docker build -t facio/build . && docker run --security-opt seccomp:../docker-security.json facio/build bash test.sh
