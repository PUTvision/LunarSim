#!/bin/bash
xhost local:root
XSOCK=/tmp/.X11-unix

docker run -it --rm \
 --gpus all \
 -e DISPLAY=$DISPLAY \
 -v $XSOCK:$XSOCK \
 -v $HOME/.Xauthority:/root/.Xauthority \
 --privileged \
 --net=host \
 lunarsim:latest bash
