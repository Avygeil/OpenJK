#!/bin/sh
git_tag=$(git log --pretty=format:'%h' -n 1)
sudo rm -rf ./build
sudo mkdir ./build
sudo docker build -t openjkded:$git_tag .
sudo docker run --rm -v $(readlink -f ./build):/root/out openjkded:$git_tag
sudo docker image rm openjkded:$git_tag
sudo chown -R --reference=./ ./build
