#!/bin/bash
###########################################################################
# Set current directory
mkdir -p UTILS/variables/
echo ${PWD} > UTILS/variables/SMC_BASE_DIR.txt
echo "${PWD}/../SMC-WORK/" > UTILS/variables/SMC_WORK_DIR.txt
if ! [ -e UTILS/variables/GNUPLOT_DEFAULT_TERMINAL.txt ]; then echo "wxt" > UTILS/variables/GNUPLOT_DEFAULT_TERMINAL.txt; fi

source UTILS/default_params.sh
set +e
print_msg "${COLOR_GREEN}Configuring SMC Simulator ..."
print_msg "$(extend "Number of Colors")> $(tput colors) ${COLOR_RED}R${COLOR_GREEN}G${COLOR_BLUE}B"
check_if_exists gcc
check_if_exists g++
check_if_exists bash
check_if_exists python
check_if_exists perl
check_if_exists gnuplot
# check_if_exists vsim
# check_if_exists vlog
# check_if_exists vlib
check_if_exists m4
check_if_exists make
check_if_exists swig
check_if_exists scons
check_if_exists telnet
check_if_exists readelf
load_model system/gem5_fullsystem_arm7.sh
check_if_exists ${HOST_CROSS_COMPILE}gcc
load_model system/gem5_fullsystem_arm8.sh
check_if_exists ${HOST_CROSS_COMPILE}gcc

print_msg "$(extend "CWD")> $PWD"

# Work directory exists
if ! [ -e $SMC_WORK_DIR ] || [ -z $SMC_WORK_DIR ] || [ $SMC_WORK_DIR == "" ]
then
    print_err "SMC_WORK_DIR is not accessible, please edit UTILS/variables/SMC_WORK_DIR.txt and write a correct absolute location in it"
    exit
fi

if [ -d $SMC_BASE_DIR ]; then
    print_msg "$(extend "SMC_BASE_DIR")> $SMC_BASE_DIR"
else
    print_err "SMC_BASE_DIR not accessible!"
fi
if [ -d $SMC_WORK_DIR ]; then
    print_msg "$(extend "SMC_WORK_DIR")> $SMC_WORK_DIR"
else
    print_err "SMC_WORK_DIR not accessible!"
fi
if [ -d $M5_PATH ]; then
    print_msg "$(extend "M5_PATH")> $M5_PATH"
else
    print_err "M5_PATH not accessible!"
fi

mkdir -p $SMC_WORK_DIR/scenarios
mkdir -p $SMC_WORK_DIR/gem5-build
mkdir -p $SMC_WORK_DIR/checkpoints
mkdir -p $SMC_WORK_DIR/traces

check_work_dir scenarios
#check_work_dir traces
check_work_dir linux_kernel
check_work_dir gem5-images
check_work_dir gem5-build
check_work_dir checkpoints
print_msg "$(extend "Sudo Command")> $(which ${SUDO_COMMAND}) ${COLOR_YELLOW}{Modify \$SUDO_COMMAND in UTILS/default_params.sh based on your environment}"

# We have enough permission to modify the work directory
cd $SMC_WORK_DIR
touch access_test
rm access_test
cd $SMC_BASE_DIR

print_msg "${COLOR_GREEN}DEPENDENCIES ... $COLOR_RED(Please check manually!)"
cat DOC/dependencies.txt
echo -e "        ${COLOR_RED}Recommended Distribution: Ubuntu 14.04.4 LTS (All dependencies resolvable with apt-get install)${COLOR_NONE}"
echo
print_msg "${COLOR_GREEN}REQUIRED VERSIONS ..."
cat DOC/versions.txt
print_msg "${COLOR_GREEN}AVAILABLE VERSIONS ..."
print_versions
echo
print_msg "Checking GNUPlot functionalities ..."
check_gnuplot_terminal x11
check_gnuplot_terminal wxt
print_msg "GNUPlot terminal set to $(cat UTILS/variables/GNUPLOT_DEFAULT_TERMINAL.txt)"

print_war "Before building, please check the dependencies mentioned above! Do you want to build the kernels now? (Y/n)"

read -e USER_INPUT
if ! [ -z $USER_INPUT ] && [ $USER_INPUT == "n" ]; then
    exit
fi

print_msg "${COLOR_GREEN}Building the kernel: linux-aarch64-gem5-20140821"
cd $SMC_WORK_DIR/linux_kernel/linux-aarch64-gem5-20140821/
#make clean
./ethz-build-kernel.sh

print_msg "${COLOR_GREEN}Building the kernel: linux-linaro-tracking-gem5-ll_20131205.0-gem5-a75e551"
cd $SMC_WORK_DIR/linux_kernel/linux-linaro-tracking-gem5-ll_20131205.0-gem5-a75e551/
#make clean
./ethz-build-kernel.sh

cd $SMC_BASE_DIR

print_msg "${COLOR_GREEN}Checking the disk images ..."
source UTILS/common.sh
check_disk_images

print_msg "${COLOR_GREEN}Configuration was successful! Do you want to build gem5 (demo scenario) now? (Y/n)"

read -e USER_INPUT
if ! [ -z $USER_INPUT ] && [ $USER_INPUT == "n" ]; then
    exit
fi

./scenarios/0-demo/1-singlepim-pagerank.sh -b
