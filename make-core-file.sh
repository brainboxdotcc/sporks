#!/bin/sh
# Produce a core file of a running bot without killing it
cd build
gcore `ps aux | grep "./bot$" | awk '{print $2}'`
