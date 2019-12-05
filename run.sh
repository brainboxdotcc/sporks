#!/bin/sh
cd build
# enable core dumps for debugging
ulimit -c unlimited
# run repeatedly until ctrl+c
while true;
do
	./bot
	../mail-core-file.sh `pwd`
done

