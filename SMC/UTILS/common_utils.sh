#!/bin/bash
# These utilities are used everywhere
#######################################################################################
set -e

# Colors
export COLOR_BOLD="\033[1m"
export COLOR_UNDERLINED="\033[4m"
export COLOR_GRAY="\033[0;37m"
export COLOR_RED="\033[1;31m"
export COLOR_GREEN="\033[1;32m"
export COLOR_YELLOW="\033[1;33m"
export COLOR_CYAN="\033[1;36m"
export COLOR_BLUE="\e[0;34m"
export COLOR_BBLUE="\e[1;34m"
export COLOR_VIOLET="\033[1;35m"
export COLOR_NONE="\033[0m"
export COLOR_PURPLE="\033[1;34m"
export SEPARATOR_LINE="********************************************************************************"
export NEWLINE="\n"

#######################################################################################
# Export an array of parameters
function export_array
{
	name=$1
	value=$2[@]
	a=("${!value}")
	c=0
# 	if (( $N_TARG_PORT > ${#a[@]} ))
# 	then
# 		if ! [ $1 == "ADDRESS_MAPPING" ] && ! [ $1 == "DEFAULT_MAPPING" ] && ! [ $1 == "ADDRESS_MASKING" ]
# 		then
# 			echo -e "$COLOR_RED Error: Not enough elements in array $1: LENGTH=${#a[@]} $COLOR_NONE"
# 			exit
# 		fi
# 	fi
	for i in "${a[@]}" ; do
		#echo $1\_$c=$i
		export $1\_$c=$i
		c=$((c+1))
	done
}

#######################################################################################
# $1 = Color
# $2 = Message
function print_colored()
{
	echo -e "${1}${2}$COLOR_NONE"
}

#######################################################################################
function print_msg()
{
	echo -e "${COLOR_BOLD}Info:${COLOR_GRAY} ${1}$COLOR_NONE"
}

#######################################################################################
function print_err()
{
	echo -e "${COLOR_RED}ERROR: ${1}$COLOR_NONE"
	exit
}

#######################################################################################
function print_war()
{
	echo -e "${COLOR_YELLOW}WARNING: ${1}$COLOR_NONE"
}

#######################################################################################
# Plot with these arguments: (this is a point-wise plot)
#  $1: file location
#  $2: title
#  $3: xlabel
#  $4: ylabel
function plot()
{
if ! [ -z $__NO_GNUPLOTS ]; then
    return
fi
gnuplot --persist <<__EOF
set title "$2"
set xlabel "$3"
set ylabel "$4"
set grid
plot "$1" with points
pause -1
__EOF
}

#######################################################################################
# Plot bar chart with these arguments:
#  $1: file location
#  $2: title
#  $3: xlabel
#  $4: ylabel
#
# Format of the input file: .gnu or .txt
# 0 "Xlabel1" Yvalue1
# 1 "Xlabel2" Yvalue2
# ...
function plot_bar()
{
if ! [ -z $__NO_GNUPLOTS ]; then
    return
fi

if [ -z $GNU_PLOT_ID ]
then
    export GNU_PLOT_ID=0
else
    export GNU_PLOT_ID=$(($GNU_PLOT_ID+1))
fi

gnuplot --persist <<__EOF
set terminal $GNUPLOT_DEFAULT_TERMINAL $GNU_PLOT_ID
set xtics border in scale 0,0 mirror rotate by -270  offset character 0, 0, 0
set xtics  norangelimit
set xtics   ()
set title "$2"
set xlabel "$3"
set ylabel "$4"
set boxwidth 0.5
set style fill solid
plot "$1" using 1:3:xtic(2) with boxes lc rgb "#449966" notitle
__EOF
}

#######################################################################################
# Plot bar graph
#  $1: stat_name
#  $2: sub_stat_name
#  --no-output : just generate the results and do not plot them
function plot_bar_chart()
{
if ! [ -z $__NO_GNUPLOTS ]; then
    return
fi

if [[ $* == *--no-output* ]]; then
	_NO_OUTPUT="TRUE"
fi
N=$1

case $2 in
    ''|*[!0-9]*) YAXIS=$2 ;;
    *) YAXIS=$3;;
esac

if [ -z $GNU_PLOT_ID ]
then
	export GNU_PLOT_ID=0
else
	export GNU_PLOT_ID=$(($GNU_PLOT_ID+1))
fi
# # First extract GNUPlot file from the $STATISTICS_FILE_NAME.csv
conv_to_gnu "$N" "$2" "$SCENARIO_HOME_DIR/$STATISTICS_FILE_NAME.csv"  "$SCENARIO_HOME_DIR/plot_$N.gnu"

if [ -z $_NO_OUTPUT ]; then
gnuplot --persist <<__EOF
set terminal $GNUPLOT_DEFAULT_TERMINAL $GNU_PLOT_ID
set xtics border in scale 0,0 mirror rotate by -270  offset character 0, 0, 0
set xtics  norangelimit
set xtics   ()
set title "$N"
set xlabel ""
set ylabel "$YAXIS"
set boxwidth 0.5
set style fill solid
plot "$SCENARIO_HOME_DIR/plot_$N.gnu" using 1:3:xtic(2) with boxes lc rgb "#449966" notitle
__EOF
fi
}

#######################################################################################
# This function gets a number and adds zeros behind it so that its total length becomes equal to length
#  (zero pad)
#  $1: the number
#  $2: length
function zpad()
{
	word=$1
	firstchat=${word:0:1}
	if [ $firstchat == "0" ] && [ ${#word} -gt 1 ]
	then
		printf "!REMOVE_ZERO_PADDING:$1!"
	else
		printf "%0$2d" $1
	fi
}

# Zero pad to float number
function zpad_float()
{
    word=$1
    word=${word/"."/" "}
    tokens=( $word )
    printf "%0$2d" ${tokens[0]}
    echo ".${tokens[1]}"
}



# Plot a 2D surface (heat map)
function surface_plot()
{
if ! [ -z $__NO_GNUPLOTS ]; then
    return
fi
_PWD=${PWD}
__PLOT_TITLE=$3
cd $1
gnuplot --persist <<__EOF
set title "${__PLOT_TITLE//_/-}"
set autoscale xfix
set autoscale yfix
set palette model CMY rgbformulae 7,5,15
set term postscript eps size 4,2 enhanced color solid
set output "$2.eps"
plot "$2.txt" matrix with image
pause -1
__EOF

ps2pdf "$2.eps" "raw_$2.pdf"
pdfcrop "raw_$2.pdf" "$2.pdf"

returntopwd
}

#######################################################################################
# plot multiple files over the same x axis ==> create EPS and PDF file outputs in $1/ folder
# $1 = Base Folder
# $2 = Plot 1
# $3 = Plot 2
# $4 = Plot 3
# $5 = Plot 4
# $6 = Plot 5
# $7 = Plot 6
# $8 = Plot 7
function multiple_plot_to_file()
{
PARENT_DIR=${PWD}
cd $1/

if ! [ -z $2 ]; then P2=$2; else P2="dummy.txt"; fi
if ! [ -z $3 ]; then P3=$3; else P3="dummy.txt"; fi
if ! [ -z $4 ]; then P4=$4; else P4="dummy.txt"; fi
if ! [ -z $5 ]; then P5=$5; else P5="dummy.txt"; fi
if ! [ -z $6 ]; then P6=$6; else P6="dummy.txt"; fi
if ! [ -z $7 ]; then P7=$7; else P7="dummy.txt"; fi
if ! [ -z $8 ]; then P8=$8; else P8="dummy.txt"; fi

gnuplot --persist <<__EOF
set title "${__PLOT_TITLE//_/-}"
set xlabel "$__PLOT_X_LABEL"
set ylabel "$__PLOT_Y_LABEL"
set grid
set style line 1 lt 1 lw 1 pt 3 lc rgb "green"
set style line 2 lt 1 lw 1 pt 3 lc rgb "magenta"
set style line 3 lt 1 lw 1 pt 3 lc rgb "red"
set style line 4 lt 1 lw 1 pt 3 lc rgb "blue"
set style line 5 lt 1 lw 1 pt 3 lc rgb "black"
set style line 6 lt 1 lw 1 pt 3 lc rgb "brown"
set style line 7 lt 1 lw 1 pt 3 lc rgb "cyan"
set term postscript eps size 4,2 enhanced color solid
set output "$__PLOT_TITLE.eps"
plot "$P2" with lines ls 1, "$P3" with lines ls 2, "$P4" with lines ls 3, "$P5" with lines ls 4, "$P6" with lines ls 5, "$P7" with lines ls 6, "$P8" with lines ls 7
pause -1
__EOF

ps2pdf "$__PLOT_TITLE.eps" "raw_${__PLOT_TITLE}.pdf"
pdfcrop "raw_${__PLOT_TITLE}.pdf" "$__PLOT_TITLE.pdf"

rm raw_* -f

cd $PARENT_DIR
}

#######################################################################################
# plot multiple files over the same x axis
# $1 = Base Folder
# $2 = Plot 1
# $3 = Plot 2
# $4 = Plot 3
# $5 = Plot 4
# $6 = Plot 5
# $7 = Plot 6
function multiple_plot()
{
if ! [ -z $__NO_GNUPLOTS ]; then
    return
fi

PARENT_DIR=${PWD}
cd $1/

if ! [ -z $2 ]; then P2=$2; else P2="dummy.txt"; fi
if ! [ -z $3 ]; then P3=$3; else P3="dummy.txt"; fi
if ! [ -z $4 ]; then P4=$4; else P4="dummy.txt"; fi
if ! [ -z $5 ]; then P5=$5; else P5="dummy.txt"; fi
if ! [ -z $6 ]; then P6=$6; else P6="dummy.txt"; fi
if ! [ -z $7 ]; then P7=$7; else P7="dummy.txt"; fi

gnuplot --persist <<__EOF
set title "$__PLOT_TITLE"
set xlabel "$__PLOT_X_LABEL"
set ylabel "$__PLOT_Y_LABEL"
set grid
plot "$P2" with lines, "$P3" with lines, "$P4" with lines, "$P5" with lines, "$P6" with lines, "$P7" with lines
pause -1
__EOF

cd $PARENT_DIR
}

#######################################################################################
# Same function as what is in the params_global.v
function EVAL_LOG2()
{
	if   [ "$1" -le "1" ];		then printf "0";
	elif [ "$1" -le "2" ];		then printf "1";
	elif [ "$1" -le "4" ];		then printf "2";
	elif [ "$1" -le "8" ];		then printf "3";
	elif [ "$1" -le "16" ];		then printf "4";
	elif [ "$1" -le "32" ];		then printf "5";
	elif [ "$1" -le "64" ];		then printf "6";
	elif [ "$1" -le "128" ];	then printf "7";
	elif [ "$1" -le "256" ];	then printf "8";
	elif [ "$1" -le "512" ];	then printf "9";
	elif [ "$1" -le "1024" ];	then printf "10";
	elif [ "$1" -le "2048" ];	then printf "11";
	elif [ "$1" -le "4096" ];	then printf "12";
	elif [ "$1" -le "8192" ];	then printf "13";
	elif [ "$1" -le "16384" ];	then printf "14";
	elif [ "$1" -le "32768" ];	then printf "15";
	else
					printf "X";
	fi
}

#######################################################################################
function print_credentials()
{
	echo "$SEPARATOR_LINE"
	echo " Smart Memory Cube (SMC) Simulator - V3.0 - 2014"
	echo " Designed by: Erfan Azarkhish (erfan.azarkhish@unibo.it)"
	echo " Collaborators: Igor Loi (igor.loi@unibo.it), Davide Rossi (davide.rossi@unibo.it)"
	echo " DEI - University of Bologna - Bologna - Italy"
	echo "$SEPARATOR_LINE"
}

# trap keyboard interrupt (control-c)
trap control_c SIGINT
trap safe_exit_process SIGTERM EXIT SIGQUIT
# run if user hits control-c
###########################################################################
control_c()
{
	printf "${COLOR_YELLOW}\n*** Ctrl+c was catched by (UTILS/common_utils.sh)! Exiting ***\n${COLOR_NONE}"
	rm -f $SMC_WORK_DIR/semaphore
	if ! [ -z $DISK_MOUNTED ]; then
		$SUDO_COMMAND  umount $SMC_WORK_DIR/temp_mount_dir/
		sync
	fi
	set +e
	export CTRL_C="TRUE"
	exit $?
}

###########################################################################
safe_exit_process()
{
	trap - SIGTERM SIGINT EXIT SIGQUIT
	# Notice: We are not deallocating the shared memory, because we may use it later (in a resumed checkpoint)
	printf "${COLOR_YELLOW}*** goodbye! ***\n${COLOR_NONE}"
	if ! [ -z $CTRL_C ]; then
#		print_msg "Killing remaining processes, because of Ctrl+C ..."
		wait
	fi
	exit
}

####################################################################################### Python parameter
# $1: type: str, int, ...
# $2: param
# $3: value
function param_python()
{
	if ! [ -z $1 ]
	then
		if [ $1 == "str" ] 
		then
			MSG="\"$3\""
		else
			MSG="$3"
		fi
	
		if ! [ -z $2 ] && ! [ -z $3 ]
		then
			printf "$2 = $MSG"
		else
			printf "$2 = Error: parameter not found"
		fi
	else
		printf "ERROR parameter type not specified!"
	fi
}

####################################################################################### Shell script parameter
# $1: type: str, none, ...
# $2: param
# $3: value
function param_shell()
{
	if ! [ -z $1 ]
	then
		if [ $1 == "str" ] 
		then
			MSG="\"$3\""
		else
			MSG="$3"
		fi
	
		if ! [ -z $2 ] && ! [ -z $3 ]
		then
			echo -e "$2=$MSG"
		else
			printf ""	# If no value is specified, just ignore the parameter
		fi
	else
		printf "ERROR parameter type not specified!"
	fi
}


####################################################################################### Cpp parameter
function param_cpp()
{
	if ! [ -z $1 ]
	then
		if [ $1 == "str" ] 
		then
			MSG="\"$3\""
		else
			MSG="$3"
		fi
	
		if ! [ -z $2 ] && ! [ -z $3 ]
		then
			printf "#define $2 $MSG"
		else
			printf "#define $2 Error: parameter not found"
		fi
	else
		printf "ERROR parameter type not specified!"
	fi
}

####################################################################################### Verilog parameter
function param_vlog()
{
	printf "Error: not implemented"
}

#######################################################################################
# Load an already available model
function load_model()
{
	source $SMC_BASE_DIR/UTILS/models/$1 $2 $3 $4 $5 $6 $7 $8
}

#######################################################################################
function conv_to_csv()
{
	$SMC_UTILS_DIR/conv_to_csv.py $1 $2
}

#######################################################################################
function conv_to_gnu()
{
	$SMC_UTILS_DIR/conv_to_gnu.py "$1" "$2" "$3" "$4"
}

#######################################################################################
function update_scenario_location()
{
	export SCENARIO_FORMAT=${__SCENARIO}/
	export SCENARIO_HOME_DIR=$SCENARIOS_DIR/$SCENARIO_FORMAT
	export SCENARIO_CASE_DIR=$SCENARIO_HOME_DIR/$__CASE/
	export STATISTICS_FILE_NAME="stats_${__SCENARIO}"
	export M5_OUT=m5out
	export M5_OUTDIR=$SCENARIO_CASE_DIR/$M5_OUT
}

#######################################################################################
function check_last_command()
{
	if [ $? -eq 0 ]; then
	printf "${COLOR_GREEN}[   OK  ]${COLOR_NONE}\n"
	else
	printf "${COLOR_RED}[ ERROR ]${COLOR_NONE}\n"
	fi
}

# Extend a message by adding dot to it to reach a specific length
#######################################################################################
# $1 = the string
# [$2] = the filler character (Can be also SPACE)
function extend()
{
	if [ -z $2 ]; then
		LEN=40;
	else
		LEN=$2
	fi
	if [ -z $3 ]; then
		CH="."
	elif [ $3 == SPACE ]; then
        CH="@"
	else
		CH=$3
	fi
	DISP=$1
	while [ ${#DISP} -lt $LEN ]; do
		DISP="$DISP$CH"
	done
	if ! [ -z $3 ] && [ $3 == SPACE ]; then # Space character is special
        DISP=${DISP//"@"/"\x20"}
    fi
	echo -e $DISP
}



#######################################################################################
function check_if_exists()
{
	set +e
	NAME=$(extend $1)
	MSG=$(which $1);
	print_msg "$NAME> $(check_last_command) $MSG"
	set -e
}

#######################################################################################
function check_work_dir()
{
	if [ -e $SMC_WORK_DIR/$1 ]
	then
		print_msg "$(extend $1)> $SMC_WORK_DIR/$1"
	else
		print_msg "$(extend $1)> ${COLOR_YELLOW}NOT FOUND"
	fi
}

#######################################################################################
function print_smc_logo()
{
	echo "███████╗███╗   ███╗ ██████╗███████╗██╗███╗   ███╗"
	echo "██╔════╝████╗ ████║██╔════╝██╔════╝██║████╗ ████║"
	echo "███████╗██╔████╔██║██║     ███████╗██║██╔████╔██║"
	echo "╚════██║██║╚██╔╝██║██║     ╚════██║██║██║╚██╔╝██║"
	echo "███████║██║ ╚═╝ ██║╚██████╗███████║██║██║ ╚═╝ ██║"
	echo "╚══════╝╚═╝     ╚═╝ ╚═════╝╚══════╝╚═╝╚═╝     ╚═╝"
}

#######################################################################################
function run_vsim()
{
	_PWD=$PWD
	cd $VSIM_BASE_DIR/
	source ./vsim.sh $1 $2 $3 $4
	returntopwd
}

#######################################################################################
function automatic_scenario()
{
	foo="${1}"
	GATHER_NAME=FALSE
	GATHER_ARG=FALSE
	SCN=""
	for (( i=2; i<${#foo}; i++ )); do
		CH=${foo:$i:1}
		if [ $CH == "/" ]; then
			GATHER_NAME=TRUE;
			continue
		fi
		if [ $CH == " " ]; then
			GATHER_ARG=TRUE;
			continue
		fi
		if [ $GATHER_NAME == TRUE ]; then
			if [[ $CH =~ ^[A-Za-z]+$ ]]; then
				GATHER_NAME=FALSE
				SCN=${SCN}$CH
			elif [[ $CH =~ ^[0-9]+$ ]]; then
				SCN=${SCN}$CH
			else
				GATHER_NAME=FALSE
			fi
		fi
		
		if [ $GATHER_ARG == TRUE ]; then
			if [[ $CH =~ ^[A-Za-z]+$ ]]; then
				SCN=${SCN}$CH
			elif [[ $CH =~ ^[0-9]+$ ]]; then
				SCN=${SCN}$CH
			else
				GATHER_ARG=FALSE
			fi
		fi
	done
	echo $SCN
}

#######################################################################################
function print_versions()
{
	echo "	$(gcc --version | grep gcc 2>&1 )"
	load_model system/gem5_fullsystem_arm7.sh
	echo "	$(${HOST_CROSS_COMPILE}gcc --version | grep gcc 2>&1 )"
	load_model system/gem5_fullsystem_arm8.sh
	echo "	$(${HOST_CROSS_COMPILE}gcc --version | grep gcc 2>&1 )"
	echo "	$(python --version 2>&1 )"
	echo "	$(perl --version | grep -m 1 perl 2>&1 )"
# 	echo "	$(vsim -version 2>&1 )"
	echo "	$(gnuplot --version 2>&1 )"
	echo "	$(swig -version | grep -m 1 SWIG 2>&1 )"
	echo "	$(m4 --version | grep -m 1 m4 2>&1 )"
	echo "	$(make --version | grep -m 1 Make 2>&1 )"
	echo "	$(readelf --version | grep readelf 2>&1 )"
	echo "	scons$(scons --version | grep -m 1 engine 2>&1 )"
}

#######################################################################################
function check_gnuplot_terminal()
{
	IS_AVAILABLE="TRUE"
	gnuplot -e "set terminal $1" > /dev/null 2>&1 || IS_AVAILABLE="FALSE"
	if [ $IS_AVAILABLE == "TRUE" ]
	then
		echo "$1" > UTILS/variables/GNUPLOT_DEFAULT_TERMINAL.txt
	else
		print_msg "  $1 terminal is not available"
	fi
}

# Create directory and touch hard disk
#######################################################################################
function create_scenario()
{
	export __CASE=$2
	export __SCENARIO_NAME=$3
	export __SCENARIO=$(automatic_scenario $1)  #$(automatic_scenario $0 $@)
	
	update_scenario_location
	
	# Touch the directories:
	# This has been added to make sure that the directories exist
	# Proper creation of directories is handled in vsim.sh and common.sh
	mkdir -p $SCENARIO_CASE_DIR
}

# This function must be called after modification of the parameters and before running
#  the simulation.
#######################################################################################
function recalculate_global_params()
{
    export DATA_WIDTH_RATIO=$(($AXI_DATA_W/$DRAM_BUS_WIDTH))
    export CH_BURST_LENGTH=`perl -l -e "print ($AXI_BURST_LENGTH*$DATA_WIDTH_RATIO)"`
    export DRAM_BANKS_PER_VAULT=$(($DRAM_BANKS_PER_DIE*$N_MEM_DIES))
    export DRAM_BANK_SIZE_w=`perl -l -e "print 2**($DRAM_column_size+$DRAM_row_size)"`           # DRAM Bank Size in Words
    export DRAM_BANK_SIZE_b=`perl -l -e "print ($DRAM_BUS_WIDTH*$DRAM_BANK_SIZE_w)"`            # DRAM Bank Size in bits
    export DRAM_BANK_SIZE_B=`perl -l -e "print ($DRAM_BANK_SIZE_b/8)"`                          # DRAM Bank Size in bytes
    export DRAM_CHANNEL_SIZE_b=`perl -l -e "print ($DRAM_BANK_SIZE_b*$DRAM_BANKS_PER_VAULT)"`   # DRAM Channel Size in bits
    export DRAM_CHANNEL_SIZE_B=`perl -l -e "print ($DRAM_CHANNEL_SIZE_b/8)"`                    # DRAM Channel Size in bytes
    export DRAM_CHANNEL_SIZE_MB=`perl -l -e "print ($DRAM_CHANNEL_SIZE_b/1024/1024/8)"`         # DRAM Channel Size in (MB) (Vault Size)
    export DRAM_LAYER_SIZE_b=`perl -l -e "print ($DRAM_BANK_SIZE_b*$DRAM_BANKS_PER_DIE)"`       # DRAM Layer Size in bits
    export DRAM_LAYER_SIZE_B=`perl -l -e "print ($DRAM_LAYER_SIZE_b/8)"`                        # DRAM Layer Size in Bytes
    export DRAM_LAYER_SIZE_MB=`perl -l -e "print ($DRAM_LAYER_SIZE_B/1024/1024)"`               # DRAM Layer Size in (MB)
    export TOTAL_MEM_SIZE_Mb=`perl -l -e "print ($N_INIT_PORT*$DRAM_CHANNEL_SIZE_b/1024/1024)"` # Total Mmeory Size in the HMC (Mb)
    export TOTAL_MEM_SIZE_MB=`perl -l -e "print ($N_INIT_PORT*$DRAM_CHANNEL_SIZE_b/1024/1024/8)"`   # Total Mmeory Size in the HMC (MB)
    export TOTAL_MEM_SIZE_B=`perl -l -e "print ($N_INIT_PORT*$DRAM_CHANNEL_SIZE_B)"`        # Total Mmeory Size in the HMC (Bytes)z
    export DRAM_tRC=`perl -l -e "print $DRAM_tRAS+$DRAM_tRP"`;                  # Notice: tRC is not an independent parameter
    export BURST_SIZE_B=`perl -l -e "print ($AXI_DATA_W * $AXI_BURST_LENGTH / 8)"`          # Maximum burst size of the system in bytes
    export SMC_BURST_SIZE_B=$BURST_SIZE_B                               # Cache line size of the system
    export NBITS_CH=$(EVAL_LOG2 $N_INIT_PORT)                           # Address Mapping Bits
    export NBITS_LB=$(EVAL_LOG2 $DRAM_BANKS_PER_VAULT)                      # Address Mapping Bits
    export NBITS_OF=$(EVAL_LOG2 $BURST_SIZE_B)                          # Address Mapping Bits
    export NBITS_RC=`perl -l -e "print ($DRAM_row_size+$DRAM_column_size-$(EVAL_LOG2 $CH_BURST_LENGTH))"`   # Row + Col bits
    export DRAM_COLUMNS_PER_ROW=`perl -l -e "print (2**($DRAM_column_size)*$DRAM_BUS_WIDTH/8/$BURST_SIZE_B)"` # Number of columns per each row
    export DRAM_ROWS_PER_BANK=`perl -l -e "print (2**($DRAM_row_size))"`                # Number of rows in the DRAM bank
    export DRAM_tREFI=`perl -l -e "print (0.001*$DRAM_tREF_PERIOD/(2**$DRAM_row_size))"`        # tREFI
    export PIM_SPM_BW_Gbps=`perl -l -e "print (${PIM_CLOCK_FREQUENCY_GHz}*32.0)"`       # SPM.Bandwidth = PIM.Frequency x 32bits
    export CLUSTER_SPM_BW_GBps=`perl -l -e "print (${PIM_CLOCK_FREQUENCY_GHz}*4.0*${PIM_SPM_BANKING_FACTOR}*(${CLUSTER_NUMSLAVECPUS}+1))"`
    export AXI_NUMBYTES=$(($AXI_DATA_W/8))
    export AXI_ID_IN=$(EVAL_LOG2 $MAX_INFLIGHT_TRANS)
    export SMC_START_ADDRESS=0x80000000     # This is fixed due to the underlying VExpress ARM based platform
    export SMC_END_ADDRESS=0x$(printf "%08X" $(($SMC_START_ADDRESS + $TOTAL_MEM_SIZE_B - 1)))
    if [ $HAVE_PIM_CLUSTER == TRUE ]; then
        export PIM_CLOCK_FREQUENCY_SCALED=`perl -l -e "print ($PIM_CLOCK_FREQUENCY_GHz * $PIPELINING_scale_factor * $CISC_scale_factor)"`        # Only for cluster simulation
    fi
}

# Mount a disk image to a local directory
#######################################################################################
# $1: disk image location
# $2: directory to mount to
function mount_disk_image()
{
	mkdir -p $2
	export DISK_MOUNTED=TRUE
	$SUDO_COMMAND  /bin/mount -o loop,offset=32256 $1 $2
}

# Copy to disk image: Mount --> Copy --> Unmount
#######################################################################################
# $1: disk image location
# $2 ... $9 files to copy to the disk image
function copy_to_disk_image()
{
    if [ -f $M5_OUTDIR/stats_gem5.txt ] && ! [ -z $CONTINUE_SIMULATION ]; then
        return
    fi

	mount_disk_image $1 $SMC_WORK_DIR/temp_mount_dir/
	sync
	sleep 1

# 	# Check if the data in extra.img has been consumed
# 	# Read the busy flag. If it is set, we must unmount, and try again some seconds later.
# 	# The data will be consumed automatically by the guest OS after some time, so all we
# 	# have to do it to wait here.
# 	if [ $1 == $GEM5_EXTRAIMAGE ]; then
# 		if [ -f $SMC_WORK_DIR/temp_mount_dir/busy ]; then
# 			print_msg "Unconsumed data in extra.img. Clearing it"
#             $SUDO_COMMAND  rm $SMC_WORK_DIR/temp_mount_dir/*
#         fi
# 		touch $SMC_WORK_DIR/busy
#  		$SUDO_COMMAND  cp $SMC_WORK_DIR/busy $SMC_WORK_DIR/temp_mount_dir/
#  	fi

	if ! [ -z $2 ]; then $SUDO_COMMAND  cp -f $2 $SMC_WORK_DIR/temp_mount_dir/	; fi
	if ! [ -z $3 ]; then $SUDO_COMMAND  cp -f $3 $SMC_WORK_DIR/temp_mount_dir/	; fi
	if ! [ -z $4 ]; then $SUDO_COMMAND  cp -f $4 $SMC_WORK_DIR/temp_mount_dir/	; fi
	if ! [ -z $5 ]; then $SUDO_COMMAND  cp -f $5 $SMC_WORK_DIR/temp_mount_dir/	; fi
	if ! [ -z $6 ]; then $SUDO_COMMAND  cp -f $6 $SMC_WORK_DIR/temp_mount_dir/	; fi
	if ! [ -z $7 ]; then $SUDO_COMMAND  cp -f $7 $SMC_WORK_DIR/temp_mount_dir/	; fi
	if ! [ -z $8 ]; then $SUDO_COMMAND  cp -f $8 $SMC_WORK_DIR/temp_mount_dir/	; fi
	if ! [ -z $9 ]; then $SUDO_COMMAND  cp -f $9 $SMC_WORK_DIR/temp_mount_dir/	; fi
	sync
    sleep 1
	$SUDO_COMMAND  umount $SMC_WORK_DIR/temp_mount_dir/
	sync
    sleep 1
	rm $SMC_WORK_DIR/temp_mount_dir/ -rf
}

# Wait to access the semaphore
#######################################################################################
# $1 file name to use as a semaphore
function semaphore_wait
{
	while [ -f $1 ]; do
		echo "Waiting for $1"
		sleep 1
	done
	echo "locked" > $1
}

# Signal free semaphore
#######################################################################################
# $1 file name to use as a semaphore
function semaphore_signal
{
	rm -f $1
}

# Copy to extra image: (extra.img) Mount --> Copy --> Unmount
#######################################################################################
# $1 ... $8 files to copy to the image
function copy_to_extra_image()
{
	# If -b or -p do nothing
	if [ -z $__BUILD_GEM5 ] && [ -z $LIST_PREVIOUS_RESULTS ]; then
	
		if [ $GEM5_AUTOMATED_SIMULATION == TRUE ] && [ $GEM5_CHECKPOINT_RESTORE == FALSE ]; then
			print_msg "Linux is booting, disk image copy skipped!"
		else
			#semaphore_wait $SMC_WORK_DIR/semaphore
			copy_to_disk_image $GEM5_EXTRAIMAGE $1 $2 $3 $4 $5 $6 $7 $8
			#semaphore_signal $SMC_WORK_DIR/semaphore
		fi
	else
		print_msg "Disk image copy skipped."
	fi
}

# Copy to main image: (extra.img) Mount --> Copy --> Unmount
#######################################################################################
# $1 ... $8 files to copy to the main image
function copy_to_main_image()
{
	copy_to_disk_image $GEM5_DISKIMAGE $1 $2 $3 $4 $5 $6 $7 $8
}

# Compile the linux kernel
#######################################################################################
function compile_linux_kernel()
{
	_PWD=${PWD}
	cd $COMPILED_KERNEL_PATH
	
	print_msg "Cleaning ..."
	make clean
	
	print_msg "Building the kernel ..."
	make ARCH=$ARCH CROSS_COMPILE=$HOST_CROSS_COMPILE gem5_defconfig
	make ARCH=$ARCH CROSS_COMPILE=$HOST_CROSS_COMPILE -j4
	#mv vmlinux vmlinux.arm64.20140821
}

# If $1 == "TRUE" return $2 otherwise return NULL
#######################################################################################
function set_if_true()
{
    if [ -z $1 ]; then
        return;
	elif [ $1 == "TRUE" ] || [ $1 == "True" ]; then
		echo $2
	fi
}

# Copy an entire directory $1 to the $SCENARIO_CASE_DIR/$2 then change current directory to $2
# After this function, call returntopwd
#######################################################################################
# $1: Directory to copy from
# $2: Directory to copy to
function clonedir()
{
	if [ -z $2 ]; then
		SRC=$1
		DST=${SRC##*/}
	else
		DST=$2
	fi
	
    if [ -z $DST ]; then # This happens when SRC directory ends with "/"
        echo
        print_msg "Cloning $1 to $SCENARIO_CASE_DIR/$DST"
        print_msg "This happens when SRC directory ends with /"
    fi
	
	export _PWD=${PWD}
	if [ -z $LIST_PREVIOUS_RESULTS ]; then
		mkdir -p $SCENARIO_CASE_DIR/$DST		# Do not use -p, because we want to catch errors in case the dir already exists
		cp -r $1/* $SCENARIO_CASE_DIR/$DST
	fi
	cd $SCENARIO_CASE_DIR/$DST
}

# Return to previous working directory stored in $_PWD
# This function is called after clonedir()
#######################################################################################
function returntopwd()
{
	cd ${_PWD}
}

# Convert size in KB, MB, GB, ... to integer value of bytes
#######################################################################################
# $1: Size in KB, MB, GB, ...
function conv_to_bytes()
{
	STR=$1
	i1=$((${#STR}-2))
	i2=$((${#STR}-1))
	NUM=${STR:0:${i1}}
	SIZE=${STR:${i1}:1}
	BYTE=${STR:${i2}:1}
	if ! [ $BYTE == "B" ]; then
		echo "Error, illegal size"
		return
	fi;
	if [ $SIZE == "K" ]; then
		echo $(($NUM * 1024))
	elif [ $SIZE == "M" ]; then
		echo $(($NUM * 1024 * 1024))
	elif [ $SIZE == "G" ]; then
		echo $(($NUM * 1024 * 1024 * 1024))
	elif [ $SIZE == "T" ]; then
		echo $(($NUM * 1024 * 1024 * 1024 * 1024))
	else
		echo ${NUM}${SIZE}
	fi
}

# Kill the process if exists, first asks user permission
#######################################################################################
# $1: process name
function kill_process()
{
	if [ "$(pidof $1)" ] 
	then
		printf "${COLOR_YELLOW}Process $1 already exists, do you want to kill it?(${COLOR_RED}y${COLOR_YELLOW}=YES, ${COLOR_GREEN}n${COLOR_YELLOW}=NO${COLOR_YELLOW})"
		read -e USER_INPUT
		if ! [ -z $USER_INPUT ] && [ $USER_INPUT == "y" ]; then
			skill $1
		else
			print_msg "Exiting because one process was busy ..."
			exit
		fi
	fi
}

# Get the memory address of the symbol $1 from the ELF binary $2 and export it in the name
# of $1=address
#######################################################################################
# $1: symbol name
# $2: elf binary name
function get_symbol_addr()
{
	if ! [ -f symbols_$2.txt ]; then
		${PIM_CROSS_COMPILE}nm -n $2 >symbols_$2.txt
	fi
	M=$(grep $1 symbols_$2.txt)
	A=0x$(cut -d ' ' -f 1 <<< "$M" )
	echo $A > $1.txt
	export $1=$A
}

# Check if the memory address of the symbol $1 from the ELF binary matches the already defined environment variable
#######################################################################################
# $1: symbol name
# $2: elf binary name
function check_symbol_addr()
{
	if ! [ -f symbols_$2.txt ]; then
		${PIM_CROSS_COMPILE}nm -n $2 > symbols_$2.txt
	fi
	M=$(grep $1 symbols_$2.txt)
	A=0x$(cut -d ' ' -f 1 <<< "$M" )
	prev=$1
	if ! [ "${!prev}" == "$A" ]; then
		print_err "Symbol has changed during compilations! $prev = ${!prev}, $A"
		exit
	fi
}

# Get the size of the symbol $1 from the ELF binary $2 and export it in the name
# of $1_size=address
#######################################################################################
# $1: symbol name
# $2: elf binary name
function get_symbol_size()
{
	if ! [ -f size.txt ]; then
		${PIM_CROSS_COMPILE}nm -S $2 > size.txt
	fi
	M=$(grep $1 size.txt)
	A=0x$(cut -d ' ' -f 2 <<< "$M" )
	echo $A > $1_size.txt
	export $1_size=$A
}


# Get the memory address, size, and binary offset of the section $1 from the ELF binary $2 
# and export it in the name of 
# $1_addr, $1_off, $1_size
#######################################################################################
# $1: section name
# $2: elf binary name
function get_section_info()
{
	if ! [ -f sections_${2}.txt ]; then
		readelf --sections --wide $2 > sections_${2}.txt
	fi
	M=$(grep $1 sections_${2}.txt)
 	MM=$(cut -d '.' -f 2 <<< "$M" )		# To pass [xx] in the beginning of the line and get to .
 	A=0x$(echo $MM | awk '{print $3}' )
	echo $A > $1_addr.txt
	export $1_addr=$A
	
 	A=0x$(echo $MM | awk '{print $4}' )
	echo $A > $1_off.txt
	export $1_off=$A
	
 	A=0x$(echo $MM | awk '{print $5}' )
	echo $A > $1_size.txt
	export $1_size=$A
}

# Dump the binary code only for a specific section in the $3.hex file
# Notice: the output file will be appended
#######################################################################################
# $1: elf binary file name
# $2: section name (without .)
# $3: output binary file
function hexdump_section()
{
	get_section_info $2 $1
	
	ADDR=$(eval echo '${'$2_addr'}')
	SIZE=$(eval echo '${'$2_size'}')
	OFF=$(eval echo '${'$2_off'}')
	printf "Section: $2\t"
	$SMC_UTILS_DIR/dump_hex.py $1 $3 $OFF $ADDR $SIZE
}


# # Dump the binary code only for a specific function in the *.hex file
# #######################################################################################
# # $1: elf binary file name
# # $2: output binary file
# # $3: start address (HEX)
# # $4: size (HEX)
# function hexdump_function()
# {
# 	$SMC_UTILS_DIR/dump_hex.py $1 $2 $3 $4
# }

# Return Ceil($1)
#######################################################################################
function ceil()
{
    echo `perl -l -e "print (int($1+0.999999))"`
}

# Return Ceil($1/$2)
#######################################################################################
function div_ceil()
{
	echo `perl -l -e "print (int($1/$2+0.999999))"`
}

# Return Round($1/$2)
#######################################################################################
function div_round()
{
	echo `perl -l -e "print (int($1/$2+0.5))"`
}

# Return Floor($1/$2)
#######################################################################################
function div_floor()
{
	echo `perl -l -e "print (int($1/$2))"`
}

# Check if section fits into the available size
#######################################################################################
# $1: current section size (integer) 3432
# $2: maximum section size (integer) 5445
# $3: message
function check_section_size()
{
	if (( "$1" > "$2" )); then
		print_err "Error: Section overflow: $3 Current(B): $1 Max(B): $2"
		exit
	fi
}

# Compare two hex numbers
#######################################################################################
# $1: number1 (hex) 0xFFFF
# $2: operator ">" ">="
# $3: number2 (hex) 0xFFFF
# $4: message
function hex_compare()
{
    S1=$1
    A1=$(echo $((16#${S1:2})))
    S2=$3
    A2=$(echo $((16#${S2:2})))

    if   [ $2 == ">" ]; then
        if (( "$A1" > "$A2" )); then
            print_err "Error: $1 $2 $3 {$4}"
        fi
    elif [ $2 == ">=" ]; then
        if (( "$A1" >= "$A2" )); then
            print_err "Error: $1 $2 $3 {$4}"
        fi
    else
        print_err "Error: unknown operator: $2"
    fi
}

# source $1 (Only if in execution mode)
# If -p do nothing
#######################################################################################
function run()
{
	if [ -z $LIST_PREVIOUS_RESULTS ]; then
		source $@
	fi
}

# check if $2 is not empty
#######################################################################################
function assert_nonempty()
{
    if [ -z $2 ]; then
        print_err "$1 should not be empty"
    fi
}

# Max of a couple of inputs
function max()
{
    MAX=0
    if ! [ -z $1 ] && [ $MAX -lt $1 ]; then MAX=$1; fi
    if ! [ -z $2 ] && [ $MAX -lt $2 ]; then MAX=$2; fi
    if ! [ -z $3 ] && [ $MAX -lt $3 ]; then MAX=$3; fi
    if ! [ -z $4 ] && [ $MAX -lt $4 ]; then MAX=$4; fi
    echo $MAX
}

# Min of two numbers
function min()
{
    if [ "$1" -lt "$2" ]; then
        echo $1
    else
        echo $2
    fi
}
