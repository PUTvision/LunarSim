#!/bin/bash
xhost local:root
XSOCK=/tmp/.X11-unix

docker build -t lunarsim .
