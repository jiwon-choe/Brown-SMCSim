#!/bin/bash

print_msg "Building App ..."
gcc -static $OFFLOADED_KERNEL_NAME.c -L./ -o main -Wall -O3 -std=c99


