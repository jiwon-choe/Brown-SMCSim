#!/bin/bash
if ! [ -z $1 ] && ( [ $1 == "7" ] || [ $1 == "8" ] ); then echo "Arch: ARM$1"; else echo "First argument must be architecture: 7 or 8 (ARM7 or ARM8)"; exit; fi

source UTILS/default_params.sh
create_scenario "$0/$*" "default" "ARMv$1 + HMC2011 + Linux (VExpress_EMM)"
####################################
load_model memory/hmc_2011.sh
load_model system/gem5_fullsystem_arm$1.sh

export GEM5_NUMCPU=1
# export GEM5_DEBUGFLAGS=Fetch,Decode,ExecAll

####################################
source ./smc.sh $*
print_msg "Done!"

