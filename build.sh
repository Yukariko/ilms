#! /bin/bash

if [ $1 == "CPU" ];
then
	make -C ./cpu clean
	make -C ./cpu
	mv ./cpu/ilms ./
else
	make -C ./gpu clean
	make -C ./gpu
	mv ./gpu/ilms ./
fi
