#!/bin/bash

set -ev


docker stop $(docker ps -a -q)
docker rm $(docker ps -a -q)
docker rmi $(docker images -f dangling=true -q)
