#!/bin/bash

echo -e "
#include \"defs.hh\"
#define $OFFLOADED_KERNEL_NAME
#define SMC_BURST_SIZE_B $SMC_BURST_SIZE_B
#define NAME \"$OFFLOADED_KERNEL_NAME\"
#define FILE_NAME \"${OFFLOADED_KERNEL_NAME}.hex\"
#define REQUIRED_MEM_SIZE 0 

$(set_if_true $DEBUG_ON "#define HOST_DEBUG_ON")
#define PASSPHRASE \"$PASSPHRASE\"
#define SALT \"$SALT\"
#define KEY \"$KEY\"
#define CPU_MEM_COST $CPU_MEM_COST
#define BLOCK_SIZE_PARAM $BLOCK_SIZE_PARAM
#define PARALLEL_PARAM $PARALLEL_PARAM
#define DK_LEN $DK_LEN

" > _app_params.h

echo -e "
#define $OFFLOADED_KERNEL_NAME
" > kernel_params.h

print_msg "Building App ..."
${HOST_CROSS_COMPILE}g++ main.cc crypto_scrypt.c crypto_scrypt_smix.c insecure_memzero.c sha256.c -static -L./ -lpimapi -lpthread -lstdc++ -o main -Wall $HOST_OPT_LEVEL


