#!/bin/bash

print_msg "Building PIM API ..."

echo -e "
#define PHY_BASE 		$PIM_ADDRESS_BASE
#define PIM_HOST_OFFSET		$PIM_HOST_OFFSET
#define PHY_SIZE 		$(conv_to_bytes $PIM_ADDRESS_SIZE)
#define PIM_INTR_REG		$PIM_INTR_REG
#define PIM_STATUS_REG		$PIM_STATUS_REG
#define PIM_COMMAND_REG		$PIM_COMMAND_REG
#define PIM_SREG		$PIM_SREG
#define PIM_VREG		$PIM_VREG
#define PIM_VREG_SIZE		$PIM_VREG_SIZE
#define PIM_M5_REG		$PIM_M5_REG
$(set_if_true $DEBUG_PIM_API "#define DEBUG_API")
" > _params.hh

${HOST_CROSS_COMPILE}g++ -Wall -c pim_task.cc pim_api.cc
${HOST_CROSS_COMPILE}ar rcs pim_api.a pim_task.o pim_api.o

