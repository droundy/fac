FROM facio/build

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
