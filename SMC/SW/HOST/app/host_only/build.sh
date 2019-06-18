#!/bin/bash

print_msg "Building App ..."
 ${HOST_CROSS_COMPILE}gcc -static $OFFLOADED_KERNEL_NAME.c -L./ -o main -Wall -O3 -std=c99


