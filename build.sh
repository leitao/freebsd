#!/usr/local/bin/bash -x
export TARGET=powerpc
export TARGET_ARCH=powerpc64le
CC=/usr/local/bin/gcc5
make -j 8 buildkernel
