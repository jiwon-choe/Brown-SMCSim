#!/bin/bash

echo -e "
#define PIM_INTR_REG $PIM_INTR_REG
#define PHY_BASE $PIM_ADDRESS_BASE
#define PHY_SIZE $(conv_to_bytes $PIM_ADDRESS_SIZE)
" > _params.h

${HOST_CROSS_COMPILE}gcc testdriver.c -static -o testdriver
