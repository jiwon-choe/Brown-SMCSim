#!/bin/bash

print_msg "Building Killer App ..."
${HOST_CROSS_COMPILE}g++ main.cc -static -L./ -lpthread -lstdc++ -o killer_app -Wall


