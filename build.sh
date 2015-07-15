#! /bin/bash

if [ $# -eq 0 ]; then
    echo "Usage: ./build.sh (CPU|GPU)"
    exit
fi

if [ $1 == "CPU" ]; then
    make -C ./cpu clean
    make -C ./cpu
    mv ./cpu/ilms ./
elif [ $1 == "GPU" ]; then
    make -C ./gpu clean
    make -C ./gpu
    mv ./gpu/ilms ./
else
    echo "Usage: ./build.sh (CPU|GPU)"
fi
