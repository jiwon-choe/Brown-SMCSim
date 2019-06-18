#!/bin/bash
set -e

source UTILS/common_utils.sh
#####################
# SCENARIO_HOME_DIR #
# SCENARIO_CASE_DIR #
#####################

#######################################################################################
function check_arguments
{
	add_positional_argument $1
	add_positional_argument $2
	add_positional_argument $3
	add_positional_argument $4
	add_positional_argument $5
	add_positional_argument $6
	add_positional_argument $7
	add_positional_argument $8
	
	if ! [ -z $__HELP ]
 	then
		show_help
	fi
	
	if ! [ -z $__LIST_SCENARIOS ]
	then
		echo -e "$COLOR_GREEN${SEPARATOR_LINE}$COLOR_NONE"
		echo "Listing all scenarios ..."
		echo
		#ls scenarios/ -a --color  -R | cat
		tree ./scenarios/
		echo -e "$COLOR_GREEN${SEPARATOR_LINE}$COLOR_NONE"
		exit
	fi
	
	if ! [ -z $__TELNET ]
	then
		telnet localhost 3456
		exit
	fi
}

#######################################################################################
function initial_checks
{
	if [ -z $__SCENARIO ]; then

		echo -e "${COLOR_BOLD}${SEPARATOR_LINE}"
		echo -e "To access help use -h"
 		echo -e "__SCENARIO is undefined, please run one of the scenarios in scenarios/ directory"
		echo -e "${SEPARATOR_LINE}$COLOR_NONE"
		exit
	fi
	
	# Kill the existing processes before running this simulation
	if [ -z $THIS_IS_THE_FIRST_CASE ] && [ -z $LIST_PREVIOUS_RESULTS ]; then
		if [ $GEM5_CLOSEDLOOP_MODELSIM == "TRUE" ]; then
			kill_process vsim
			kill_process vsimk
			kill_process vish
            kill_process gem5.fast
            kill_process gem5.debug
		fi
	fi
}

#######################################################################################
function add_positional_argument
{
	ARG=$1
	if ! [ -z $ARG ]
	then
        if [ "$ARG" = "--config" ]
        then
            ./UTILS/configure.sh
            exit
        elif [ "$ARG" = "-s" ]
        then
            export __LIST_SCENARIOS="TRUE"
		elif [ "$ARG" = "-b" ]
		then
			export __BUILD_GEM5="TRUE"
		elif [ "$ARG" = "-r" ]
		then
			export __REMOVE_CHECKPOINT="TRUE"
		elif [ "$ARG" = "-d" ]
		then
			export __DEBUG_GEM5="TRUE"
        elif [ "$ARG" = "-o" ]
        then
            export SHOW_STDOUT="TRUE"
        elif [ "$ARG" = "-c" ]
        then
            export CONTINUE_SIMULATION="TRUE"
        elif [ "$ARG" = "-u" ]
        then
            export __UPDATE_ONLY="TRUE"
        elif [ "$ARG" = "-q" ]
        then
            export __NO_GNUPLOTS="TRUE"
		elif [ "$ARG" = "-p" ]
		then
			export LIST_PREVIOUS_RESULTS="TRUE"
		elif [ "$ARG" = "-h" ]
		then
			export __HELP="TRUE"
		elif [ "$ARG" = "-t" ]
		then
			export __TELNET="TRUE"
		elif [ "$ARG" = "-ignore" ] # Ignore this argument
		then
			IGNORE=TRUE
		elif [ "$ARG" = "7" ] || [ "$ARG" = "8" ]
		then
			IGNORE=TRUE
		elif [ "$ARG" = "-v" ]
		then
			export __VIEW_CONFIGS="TRUE"
		else
			echo "Illegal argument! [$ARG]"
			export SHOW_HELP="TRUE"
			exit
		fi
		#echo "   +argument: $ARG - $meaning"
	fi
}

#######################################################################################
function setup_directory_structure
{
	if [ $SMC_BASE_DIR -ef $SMC_WORK_DIR ]
	then
		print_err "SMC_BASE_DIR must be different from SMC_WORK_DIR!"
		exit
	fi

	if ! [ -d $GEM5_BASE_DIR ]
	then
		print_err "Gem5 directory not found! $GEM5_BASE_DIR"
	fi
	
	if ! [ -z $__BUILD_GEM5 ]
	then
		print_msg "Creating link gem5-build --> $GEM5_BUILD_DIR"
		mkdir -p $GEM5_BUILD_DIR
		_PWD=${PWD}
		cd $GEM5_BASE_DIR
		rm -f build
		ln -sf $GEM5_BUILD_DIR build
		returntopwd
	fi
	
	if ! [ -z $__SCENARIO ] && ! [ -z $__CASE ]
	then
		update_scenario_location
		if ! [ -z $__VIEW_CONFIGS ]
		then
			view_configs
			exit
		fi
		
		if [ $SCENARIO_HOME_DIR = "" ] || [ $SCENARIO_HOME_DIR == "/" ] || [ $SCENARIO_HOME_DIR == "." ] ||
		   [ $SCENARIO_HOME_DIR -ef $SMC_BASE_DIR ] || [ $SCENARIO_HOME_DIR -ef $SMC_BASE_DIR ] || [ ${#SCENARIO_HOME_DIR} -lt 5 ]
		then
			print_err "There is a problem with SCENARIO_HOME_DIR=$SCENARIO_HOME_DIR"
			exit
		fi
		
		if ! [ -z $__REMOVE_CHECKPOINT ]; then
			printf "${COLOR_YELLOW}Do you want to remove the checkpoints for current scenario? (${COLOR_RED}ENTER${COLOR_YELLOW}=YES, ${COLOR_GREEN}Ctrl+C${COLOR_YELLOW}=NO, ${COLOR_GREEN}n${COLOR_YELLOW}=NO${COLOR_YELLOW}) "
			read -e USER_INPUT
			if ! [ -z $USER_INPUT ] && [ $USER_INPUT == "n" ]; then
				exit
			else
				rm -rf $SMC_WORK_DIR/checkpoints/$__SCENARIO/
				print_msg "Checkpoints were removed completely. (Dir: $SMC_WORK_DIR/checkpoints/$__SCENARIO/)"
				exit
			fi
		fi

		if [ -z $THIS_IS_THE_FIRST_CASE ] && [ -z $LIST_PREVIOUS_RESULTS ] && [ -z $CONTINUE_SIMULATION ]
		then
			print_colored ${COLOR_GREEN} "SCENARIO:[$__SCENARIO] {$__SCENARIO_NAME}${COLOR_NONE} $SCENARIO_HOME_DIR"
			echo
			printf "${COLOR_YELLOW}You are about to clear the whole scenario location, do you proceed? (${COLOR_RED}ENTER${COLOR_YELLOW}=YES, ${COLOR_GREEN}Ctrl+C${COLOR_YELLOW}=NO, ${COLOR_GREEN}n${COLOR_YELLOW}=NO${COLOR_NONE}) "
			read -e USER_INPUT
			if ! [ -z $USER_INPUT ] && [ $USER_INPUT == "n" ]; then
				exit
			fi
			
			if [ $GEM5_CHECKPOINT_RESTORE == "TRUE" ] && [ $GEM5_CLOSEDLOOP_MODELSIM == "TRUE" ]; then
				print_war "Not cleaning the scenario location, because we are resuming a checkpoint in closed-loop simulation"
			else
				rm -rf $SCENARIO_HOME_DIR
			fi
		
			export THIS_IS_THE_FIRST_CASE="TRUE"
		fi
		if [ -z $__UPDATE_ONLY ]; then
			print_colored ${COLOR_GREEN} "CASE:[$__CASE]${COLOR_NONE} $SCENARIO_CASE_DIR"
		fi
		

		mkdir -p $SCENARIO_CASE_DIR
		
	else
		print_err "Scenario and case must be specified correctly!"
		exit
	fi
	
	export HAVE_DRAMSIM="FALSE";
	if ! [ -z $GEM5_PLATFORM ] && ! [ -z $GEM5_SIM_MODE ] && [ -z $LIST_PREVIOUS_RESULTS ]
	then
		mkdir -p $M5_OUTDIR
		if [ $HAVE_PIM_CLUSTER == TRUE ]; then
            mkdir -p $M5_OUTDIR/terminals/
        fi
 		cp $GEM5_UTILS_DIR/python/*.py $M5_OUTDIR

		_PWD=${PWD}
 		cd $SCENARIO_CASE_DIR
 		ln -sf $GEM5_BUILD_DIR build
 		ln -sf $GEM5_BASE_DIR/configs configs
 		returntopwd
 		
		if [ $GEM5_MEMTYPE == "dramsim2" ] || [ $GEM5_MEMTYPE == "DRAMSim2" ]; then
			export HAVE_DRAMSIM="TRUE"
			DRAMSIM_FLAG="_DRAMSIM"
			rm $GEM5_BASE_DIR/ext/dramsim2/DRAMSim2/results -rf
			mkdir -p $M5_OUTDIR/dramsim2
			_PWD=${PWD}
			cd $GEM5_BASE_DIR/ext/dramsim2/DRAMSim2/
			ln -sf $M5_OUTDIR/dramsim2 results
			returntopwd
		else
			DRAMSIM_FLAG=""
			export HAVE_DRAMSIM="FALSE"
		fi
	
		export GEM5_CONFIG=$GEM5_PLATFORM${DRAMSIM_FLAG}/gem5.$GEM5_SIM_MODE
	fi
	
	if [ $HAVE_PIM_DEVICE == "TRUE" ]; then
		_PWD=${PWD}
		cd $GEM5_BASE_DIR/src/mem	# --->
		rm -f ethz_pim_definitions.h
		ln -sf $PIM_SW_DIR/resident/definitions.h ethz_pim_definitions.h
		returntopwd			# <---
		
		# Extract the page-shift of the operating system
		if [ $GEM5_PLATFORM == "ARM" ]; then
			M=$(grep -m 1 "#define PAGE_SHIFT" $COMPILED_KERNEL_PATH/arch/arm/include/asm/page.h)
			tokens=( $M )
			export OS_PAGE_SHIFT=${tokens[2]}
			if [ -z $OS_PAGE_SHIFT ]; then
				print_err "Could not extract PAGE_SHIFT from the kernel source!"
				exit
			fi
		else
			print_err "Unsupported platform: $GEM5_PLATFORM!"
		fi
	fi
	
}

#######################################################################################
function show_help
{
	echo -e "$COLOR_GREEN${SEPARATE}$COLOR_NONE"
	print_smc_logo
	echo " Smart Memory Cube (SMC) Simulator"
	echo " Designed by: Erfan Azarkhish (erfan.azarkhish@unibo.it)"
    echo " DEI - University of Bologna - Bologna - Italy"
    echo " IIS - University of ETH Zurich - Switzerland"
	echo " "
	echo " execution: run one of the scenarios in UTILS/scenarios/ folder"
	echo " Switches:"
	echo "    *****************************"
    echo "     --config  configure environment (first use)"
    echo "     -s        list all scenarios"
	echo "     -h        show help"
	echo "     -b        build gem5"
	echo "     -d        debug gem5"
    echo "     -o        show STDOUT"
    echo "     -c        continue simulation (do not clean the work dir)"
	echo "     -p        list previous results (keep them)"
	echo "     -u        update the env. vars only"
	echo "     -t        run telnet"
	echo "     -v        view configuration files"
    echo "     -r        remove checkpoints for this scenario"
    echo "     -q        [quiet] do not generate any gnu plots"
	echo " For scenarios see ./scenarios.sh"
	echo -e "$COLOR_GREEN${SEPARATOR_LINE}$COLOR_NONE"
	exit
}

#######################################################################################
# Gather software stats from system.terminal and append them to the stats.txt file
function gather_software_stats
{
	set +e
	echo "---------- Software Statistics ------------------" >> $M5_OUTDIR/stats.txt
	grep "softwarestat." $M5_OUTDIR/system.terminal >> $M5_OUTDIR/stats.txt
	set -e
}

#######################################################################################
# Gather software stats from gem5.log
function gather_software_stats_from_log
{
    set +e
    echo "---------- Software Statistics ------------------" >> $M5_OUTDIR/stats.txt
    grep "softwarestat." $M5_OUTDIR/gem5.log >> $M5_OUTDIR/stats.txt
    set -e
}

#######################################################################################
# Get single cluster stat
# $1 = Slave CPU ID
# $2 = Stat Name
function get_cluster_stat
{
    if   [ $CLUSTER_NUMSLAVECPUS -eq 0 ]; then
        return
    elif   [ $CLUSTER_NUMSLAVECPUS -eq 1 ]; then
        echo $(get_stat "timestamp1.system.slavecpus.$2")
        return
    elif   [ $CLUSTER_NUMSLAVECPUS -lt 10 ]; then
        digits=1
    elif [ $CLUSTER_NUMSLAVECPUS -lt 100 ]; then
        digits=2
    elif [ $CLUSTER_NUMSLAVECPUS -lt 1000 ]; then
        digits=3
    else
        print_err "Illegal CLUSTER_NUMSLAVECPUS"
    fi
    echo $(get_stat "timestamp1.system.slavecpus$(zpad $1 $digits).$2")
}

#######################################################################################
# Accumulate single cluster stat
# $1 = Slave CPU ID
# $2 = Stat Name
function accumulate_cpu_stat
{
    if   [ $CLUSTER_NUMSLAVECPUS -eq 0 ]; then
        return
    elif   [ $CLUSTER_NUMSLAVECPUS -eq 1 ]; then
        res=$2
        export $res=$((${!res} + $(get_stat "timestamp1.system.slavecpus.cpu.op_class::$res")))
        return
    elif   [ $CLUSTER_NUMSLAVECPUS -lt 10 ]; then
        digits=1
    elif [ $CLUSTER_NUMSLAVECPUS -lt 100 ]; then
        digits=2
    elif [ $CLUSTER_NUMSLAVECPUS -lt 1000 ]; then
        digits=3
    else
        print_err "Illegal CLUSTER_NUMSLAVECPUS"
    fi
    res=$2
    export $res=$((${!res} + $(get_stat "timestamp1.system.slavecpus$(zpad $1 $digits).cpu.op_class::$res")))
}

#######################################################################################
# Gather cluster stats (only for the cluster simulation
function gather_cluster_stats
{
    if [ $GATHER_CLUSTER_STATS == FALSE ]; then
        print_war "Skipping statistics because GATHER_CLUSTER_STATS=FALSE"
        return
    fi

    set +e
    echo "---------- Cluster Statistics ------------------" >> $M5_OUTDIR/stats.txt
    
    SimdFloatMultAcc=0
    SimdFloatMult=0
    SimdFloatAdd=0
    IntAlu=0
    MemRead=0
    MemWrite=0
    avg_idle=0
    avg_cpi=0
    vfpu_total_macs=0
    
    ########## CPU STATS ##########
    for (( c=0; c< $CLUSTER_NUMSLAVECPUS; c++ ))
    do
          accumulate_cpu_stat $c SimdFloatMultAcc
          accumulate_cpu_stat $c SimdFloatMult
          accumulate_cpu_stat $c SimdFloatAdd
          accumulate_cpu_stat $c IntAlu
          accumulate_cpu_stat $c MemRead
          accumulate_cpu_stat $c MemWrite   
          idle=$(get_cluster_stat $c cpu.idle_fraction)
          set_stat cluster.slavecpu$c.idle_fraction $idle Percent
          avg_idle=`perl -l -e "print($avg_idle + $idle)"`
          
          cpi=`perl -l -e "print($(get_cluster_stat $c cpu.numCycles) / $(get_cluster_stat $c cpu.committedInsts))"`
          avg_cpi=`perl -l -e "print($avg_cpi + $cpi)"`
          set_stat cluster.slavecpu$c.cpi $cpi CPI_not_scaled

          branch_ratio=`perl -l -e "print($(get_cluster_stat $c cpu.Branches) / $(get_cluster_stat $c cpu.committedInsts) * 100.0)"`
          instructions_per_loop=`perl -l -e "print($(get_cluster_stat $c cpu.committedInsts) / $(get_cluster_stat $c cpu.Branches))"`
          set_stat cluster.slavecpu$c.branch_ratio $branch_ratio Percent
          set_stat cluster.slavecpu$c.instructions_per_loop $instructions_per_loop Instrs
          
          # VFPU stats
          vfpu_macs=$(get_value_from_file timestamp1.slavecpu$c.vfpu_num_macs 1 vfpu.stats.txt)
          vfpu_total_macs=$(($vfpu_total_macs + $vfpu_macs ))
          
    done
    
    TotalMACs=$(($SimdFloatMultAcc + $vfpu_total_macs))
    avg_idle=`perl -l -e "print($avg_idle/$CLUSTER_NUMSLAVECPUS*100.0)"`
    avg_cpi=`perl -l -e "print($avg_cpi/$CLUSTER_NUMSLAVECPUS/$PIPELINING_scale_factor)"`

    set_stat cluster.vfpu.total_macs           $vfpu_total_macs  MAC 
    set_stat cluster.op_class.SimdFloatMultAcc $SimdFloatMultAcc MAC 
    set_stat cluster.op_class.SimdFloatMult    $SimdFloatMult    MULT
    set_stat cluster.op_class.SimdFloatAdd     $SimdFloatAdd     ADD
    set_stat cluster.op_class.IntAlu           $IntAlu           INT
    set_stat cluster.op_class.MemRead          $MemRead          READ
    set_stat cluster.op_class.MemWrite         $MemWrite         WRITE
    set_stat cluster.avg_idle                  $avg_idle         Percent
    set_stat cluster.avg_cpi_scaled            $avg_cpi          Cycles
    set_stat cluster.total_macs                $TotalMACs        MAC
    set_stat "************"
    
    ##########  MEMORY STATS ##########

    rd_bw=0
    wr_bw=0
    l_tot=0
    if [ -z $IGNORE_DRAM_STATS ]; then
        for (( m=0; m< $N_INIT_PORT; m++ ))
        do
            # Read Bandwidth
            s=$(get_stat timestamp1.system.mem_ctrls$(zpad $m 2).bw_read::total)
            rd_bw=`perl -l -e "print($rd_bw + $s)"`
            
            # Write Bandwidth
            s=$(get_stat timestamp1.system.mem_ctrls$(zpad $m 2).bw_write::total)
            wr_bw=`perl -l -e "print($wr_bw + $s)"`
            
            # Queueing Latency
            l=$(get_stat timestamp1.system.mem_ctrls$(zpad $m 2).totQLat)
            l_tot=`perl -l -e "print($l_tot + $l)"`
            
        done
    fi
    
    duration=$(get_stat timestamp1.sim_ticks)
    rd_bw=`perl -l -e "print($rd_bw /1000.0/1000.0/1000.0)"`
    wr_bw=`perl -l -e "print($wr_bw /1000.0/1000.0/1000.0)"`
    l_tot=`perl -l -e "print(1.0* $l_tot /$N_INIT_PORT/1000.0/1000.0/1000.0)"`
    bw=`perl -l -e "print($wr_bw + $rd_bw)"`
    gmacs_per_second=`perl -l -e "print($TotalMACs / $duration * 1000.0)"`
    if ! [ "$wr_bw" == 0 ] || ! [ "$rd_bw" == 0 ]; then
        operational_intensity=`perl -l -e "print($gmacs_per_second / ($wr_bw + $rd_bw))"`
    fi
    
    rbw=$(get_stat timestamp1.system.Dmon.averageReadBandwidth);
    wbw=$(get_stat timestamp1.system.Dmon.averageWriteBandwidth);    
    dma_bw=`perl -l -e "print(($rbw + $wbw)/1000.0/1000.0/1000.0)"`
    
    rbw=$(get_stat timestamp1.system.Hmon.averageReadBandwidth);
    wbw=$(get_stat timestamp1.system.Hmon.averageWriteBandwidth);    
    direct_bw=`perl -l -e "print(($rbw + $wbw)/1000.0/1000.0/1000.0)"`
    
    set_stat cluster.bandwidth_write            $wr_bw "GigaBytes/Sec"
    set_stat cluster.bandwidth_read             $rd_bw "GigaBytes/Sec"
    set_stat cluster.bandwidth_total            $bw "GigaBytes/Sec"
    set_stat vaults.qlatency_total              $l_tot "(ms)"
    set_stat "************"
    set_stat cluster.bandwidth_dma              $dma_bw "GigaBytes/Sec"
    set_stat cluster.bandwidth_direct           $direct_bw "GigaBytes/Sec"
    set_stat "************"
    set_stat cluster.gmacs_per_second           $gmacs_per_second "GigaMACs/Sec"
    set_stat cluster.operational_intensity      $operational_intensity "MAC/Byte"
    
    #### ROOFLINE MODEL ####
    
    
    
    set -e
}

#######################################################################################
# Report statistics given by globally exported variable GEM5_STATISTICS
function report_gem5_stats
{
    if ! [ -z $AUTOMATIC_REPORT_GEM5_STATS ]; then
        return;
    fi

    if [ $HAVE_PIM_DEVICE == TRUE ]; then
        alias_gem5_statistics
        calculate_power_consumption
    else
        cp $SCENARIO_CASE_DIR/m5out/stats_gem5.txt $SCENARIO_CASE_DIR/m5out/stats.txt
    fi

    for S in ${GEM5_STATISTICS[*]}
    do
        report_gem5_stat $S
    done
}

#######################################################################################
# Fetch a statistic value from $M5_OUT/stats.txt and report it in a text file, also on the screen
function report_gem5_stat
{
	#echo Reporting statistic $1 in $SCENARIO_CASE_DIR
 	#grep "$1" $M5_OUTDIR/stats.txt || echo -e "${COLOR_RED}Stat $1 not found!${COLOR_NONE}"
    #grep "$1" $M5_OUTDIR/stats.txt > $M5_OUTDIR/$1.stat
    echo $(get_stat "$1") > $M5_OUTDIR/$1.stat
    print_colored $COLOR_BBLUE "Stat: {$1}:  $(get_stat "$1")"
}

#######################################################################################
function gather_gem5_stats
{
	if [ $GEM5_CPUTYPE == atomic ]; then
		print_war "Gathering statistics in the atomic simulation mode is not a good idea!"	
	fi

 	echo "Gathering stats ($STATISTICS_FILE_NAME.txt) ..."
	clear_gem5_stats
	for S in ${GEM5_STATISTICS[*]}
	do
		echo $S
		gather_gem5_stat $S
	done
  	echo "Generating CSV file ($STATISTICS_FILE_NAME.csv)"
  	conv_to_csv "$SCENARIO_HOME_DIR/$STATISTICS_FILE_NAME.txt" "$SCENARIO_HOME_DIR/$STATISTICS_FILE_NAME.csv"
	current_date=$(date)
 	echo "Finished [$current_date]"
}

#######################################################################################
# Gather $1 statistic value ($1.stat) in a $STATISTICS_FILE_NAME.txt file
function gather_gem5_stat
{
	#echo "Gathering stat: $1"
	echo >> $SCENARIO_HOME_DIR/$STATISTICS_FILE_NAME.txt

	for i in $(ls -d $SCENARIO_HOME_DIR/*/);
	do 
		#echo ${i%%/}
		for j in $(ls ${i%%}/$M5_OUT/$1.stat);
		do
			echo " ${j}" >> $SCENARIO_HOME_DIR/$STATISTICS_FILE_NAME.txt
		done
	done
	
	for i in $(ls -d $SCENARIO_HOME_DIR/*/);
	do 
		for j in $(ls ${i%%}/$M5_OUT/$1.stat);
		do
			cat ${j} >> $SCENARIO_HOME_DIR/$STATISTICS_FILE_NAME.txt
		done
	done
}

#######################################################################################
# Clear statistics
function clear_gem5_stats
{
	#echo Clearing statistics of $SCENARIO_HOME_DIR
	echo > $SCENARIO_HOME_DIR/$STATISTICS_FILE_NAME.txt
}

#######################################################################################
function view_configs()
{
	echo -e "$COLOR_GREEN${SEPARATOR_LINE}$COLOR_NONE"
	
	NAME=_gem5_params.py
	echo -e "$COLOR_GREEN---------------------- $NAME $COLOR_NONE"
	cat $M5_OUTDIR/$NAME
	
	NAME=_smc_params.txt
	echo -e "$COLOR_GREEN---------------------- $NAME $COLOR_NONE"
	cat $M5_OUTDIR/$NAME
	
	echo -e "$COLOR_GREEN${SEPARATOR_LINE}$COLOR_NONE"
	exit
}

#######################################################################################
function finalize_gem5_simulation()
{
	if [ $GEM5_AUTOMATED_SIMULATION == TRUE ] && [ $GEM5_CHECKPOINT_RESTORE == FALSE ]; then
		print_msg "Reporting statistics skipped."
		return
	fi
	rm $SCENARIO_HOME_DIR/RTL -rf
	gather_gem5_stats
}

#######################################################################################
# Check for the existence of the empty disk image ($GEM5_EXTRAIMAGE), if not create an empty one
# Notice: Remove $SMC_VARS_DIR/DISK_IMAGES_CHECKED.txt to create a fresh disk image and copy the 
#  required files automatically
function check_disk_images()
{
	if [ -e $SMC_VARS_DIR/DISK_IMAGES_CHECKED.txt ]; then
		return;
	fi
	print_msg "Preparing the disk images ..."

	if ! [ -e $GEM5_EXTRAIMAGE ]; then
		print_msg "$GEM5_EXTRAIMAGE not found, creating a new empty image ..."
		$GEM5_BASE_DIR/util/gem5img.py init $GEM5_EXTRAIMAGE 512	# Size is in Mega Bytes
		
		print_msg "testing $GEM5_EXTRAIMAGE ..."
		echo "This image is used for file transfer among HOST and GUEST environments!" >> info.txt
		copy_to_extra_image info.txt
		rm info.txt
		print_msg "Done!"
	fi
	
	if ! [ -e $GEM5_DISKIMAGE ]; then
		print_err "Gem5 disk image not found! $GEM5_DISKIMAGE"
		exit
	else
		##########ARM7###########################
		load_model system/gem5_fullsystem_arm7.sh
		cp $SMC_UTILS_DIR/models/system/get_data_from_host_${ARCH_ALIAS}.sh $SMC_WORK_DIR/get
		copy_to_main_image $SMC_WORK_DIR/get
		##########ARM8###########################
		# Creating the automated boot scripts (Notice that for ARM7 this mechanism is already present
		#  in the original images, but for ARMv8, we should do it here
		load_model system/gem5_fullsystem_arm8.sh
		cp $SMC_UTILS_DIR/models/system/get_data_from_host_${ARCH_ALIAS}.sh $SMC_WORK_DIR/get
		copy_to_main_image $SMC_WORK_DIR/get
				
		mkdir $SMC_WORK_DIR/temp_mount_dir
		mount_disk_image $GEM5_DISKIMAGE $SMC_WORK_DIR/temp_mount_dir
		sync
		$SUDO_COMMAND cp $SMC_UTILS_DIR/arm8-auto-root-login $SMC_WORK_DIR/temp_mount_dir/usr/bin/auto-root-login
		$SUDO_COMMAND cp $SMC_UTILS_DIR/arm8-S99automated $SMC_WORK_DIR/temp_mount_dir/etc/rc5.d/S99automated
		$SUDO_COMMAND chmod +x $SMC_WORK_DIR/temp_mount_dir/etc/rc5.d/S99automated
		$SUDO_COMMAND chmod +x $SMC_WORK_DIR/temp_mount_dir/usr/bin/auto-root-login
		sync
		$SUDO_COMMAND umount $SMC_WORK_DIR/temp_mount_dir/
		sync
		rm $SMC_WORK_DIR/temp_mount_dir/ -rf
		#########################################
		rm $SMC_WORK_DIR/get
	fi

    echo "If you had problems with the disk images, just delete this file. Disk images will be automatically manipulated." > $SMC_VARS_DIR/DISK_IMAGES_CHECKED.txt
    print_msg "Disk images were prepared successfully! To create fresh disk images just remove: $SMC_VARS_DIR/DISK_IMAGES_CHECKED.txt"
    if ! [ $__SCENARIO == default ]; then
        print_msg "Now exiting ..."
        print_msg "((( Please rerun your script now )))"
        exit 0  
    fi
}

#######################################################################################
# Check external tools
function check_external_tools
{
    if [ -f $GEM5_BUILD_DIR/$CACTI/cacti ]; then
        print_msg "CACTI: $GEM5_BUILD_DIR/$CACTI (For automatic rebuild, remove this directory)"
    else
        rm -rf $GEM5_BUILD_DIR/$CACTI/
        cp -r $GEM5_UTILS_DIR/$CACTI $GEM5_BUILD_DIR/
        print_msg "Building CACTI ..."
        _PWD=${PWD}
        cd $GEM5_BUILD_DIR/$CACTI
        make
        returntopwd
    fi
}

#######################################################################################
# Calculate power consumption using different tools
function calculate_power_consumption
{
    _PWD=${PWD}
    cd $M5_OUTDIR
    python $GEM5_UTILS_DIR/python/jsonparser/jsonparser.py config.json > params.txt
    
    call_cacti
    calculate_power_for_timestamp 6 pim
    calculate_power_for_timestamp 8 host
    
    returntopwd
}
#######################################################################################
# $1: timestamp
# $2: label
function calculate_power_for_timestamp
{
	print_msg "**** Calculating power consumption for timestamp$1 - $2 ****"
	#************************************
    print_msg "  Link power: [Maya Gokhale]"
    link_r_bw=$(get_stat timestamp$1.system.Hmon.averageReadBandwidth)     # Bytes/sec.
    link_w_bw=$(get_stat timestamp$1.system.Hmon.averageWriteBandwidth)     # Bytes/sec.
    link_dyn_pwr=`perl -l -e "print (($link_r_bw + $link_w_bw) * 8.0 * $SERIALLINKS_ENERGY_PER_BIT / 1000.0 / 1000.0 / 1000.0 )"` # mW
    
    if [ $1 == 6 ]; then
        # When PIM executes, we can completey shutdown the serial links by means of power gating.
        # In this mode, we can assume that the power consumption of the links is negligible.
        # [Memory-centric System Interconnect Design with Hybrid Memory Cubes]    
        # Also statistics show that the host mostly hit in the L2 cache and does not need to access the main memory.
        #set_stat link_power.$2  $link_dyn_pwr mW
        set_stat link_power.$2  0 mW
    else
        # When host is executing, we cannot turn off the serial links, so IDLE power is used by the NULL flits even 
        # if we are not using the maximum bandwidth
        link_pwr=`perl -l -e "print ($link_dyn_pwr + $SERIALLINKS_IDLE_POWER)"` # mW
        set_stat link_power.$2  $link_pwr mW
    fi
    
    #************************************
    print_msg "  DRAM Power: DRAMPower"
    dram_pwr=0
    total_bw=0
    c1=0
    c2=0
    c3=0
    for (( c=0; c<$N_INIT_PORT; c++ )); do
        #Note: we consider the amount of physical memory that we actually used for our task.
        # So, we don't pay for the power consumed in the unused DRAM memory
        p=$(get_stat  timestamp$1.system.mem_ctrls$(zpad $c 2).averagePower::0)
        dram_util=`perl -l -e "print ($(get_stat softwarestat.task.pagesize) / ($TOTAL_MEM_SIZE_B))"` # mW {Power per rank * Number of Ranks}
        dram_pwr=`perl -l -e "print ($dram_pwr + ($p * $N_MEM_DIES ))"` # mW {Power per rank * Number of Ranks}
        
        # Total bandwidth to calculate vault controller's power consumption
        bw=$(get_stat  timestamp$1.system.mem_ctrls$(zpad $c 2).bw_total::total)
        total_bw=`perl -l -e "print ($total_bw + $bw)"`

        # Number of atomic operations done in vaults
        if [ $1 == 6 ]; then
            c1=$(($c1 + $(get_stat  timestamp$1.system.mem_ctrls$(zpad $c 2).atomic_fadd_count)))
            c2=$(($c2 + $(get_stat  timestamp$1.system.mem_ctrls$(zpad $c 2).atomic_min_count)))
            c3=$(($c3 + $(get_stat  timestamp$1.system.mem_ctrls$(zpad $c 2).atomic_inc_count)))
        fi
    done
    
    set_stat dram_power.$2  $dram_pwr mW
    task_pwr=`perl -l -e "print ($dram_pwr * $dram_util)"` # mW {Power per rank * Number of Ranks}
    set_stat dram_task_power.$2  $task_pwr mW
    vaultctr_pwr=`perl -l -e "print ( $total_bw * 8.0 * $VAULTCTRL_ENERGY_PER_BIT /1000.0 /1000.0 /1000.0 )"` # mW {Vault Controllers}
    set_stat vaultctrl_power.$2  $vaultctr_pwr mW
    T=$(get_stat timestamp$1.sim_seconds) # seconds
    atomic_pwr=`perl -l -e "print ((($c1*$ATOMIC_FADD_ENERGY) + ($c2*$ATOMIC_MIN_ENERGY) + ($c1*$ATOMIC_INC_ENERGY))/$T/ 1000.0 / 1000.0 / 1000.0)"` # mW {atomic operations}
    set_stat atomic_power.$2  $atomic_pwr mW

    #************************************
    print_msg "  Cache Power: CACTI"
    if [ $HAVE_L2_CACHES == TRUE ]; then
        set_stat l2_power.$2 $(calculate_cache_power $1 system.l2 cacti_l2.txt)
    fi
    if [ $HAVE_L1_CACHES == TRUE ]; then
        for (( c=0; c<$GEM5_NUMCPU; c++ )); do
            set_stat cpu$c.li_power.$2 $(calculate_cache_power $1 system.cpu$c.icache cacti_li.txt)
            set_stat cpu$c.ld_power.$2 $(calculate_cache_power $1 system.cpu$c.dcache cacti_ld.txt)
        done
    fi
    #************************************
    print_msg "  Interconnect Power: Logic Synthesis"
    p=$(get_stat  timestamp$1.system.smcxbar.pkt_size::total)
    t=$(get_stat  timestamp$1.sim_seconds)
    smcxbar_power=`perl -l -e "print ($p * 8.0 / $t * $SMCXBAR_ENERGY_PER_BIT /1000.0/1000.0/1000.0)"`  #mW
    set_stat smcxbar_power.$2  $smcxbar_power mW
    
    p=$(get_stat  timestamp$1.system.membus.pkt_size::total)
    membus_power=`perl -l -e "print ($p * 8.0 / $t * $MEMBUS_ENERGY_PER_BIT /1000.0/1000.0/1000.0)"`  #mW
    set_stat membus_power.$2  $membus_power mW
    
    p=$(get_stat  timestamp$1.system.tol2bus.pkt_size::total)
    tol2bus_power=`perl -l -e "print ($p * 8.0 / $t * $TOL2BUS_ENERGY_PER_BIT /1000.0/1000.0/1000.0)"`  #mW
    set_stat tol2bus_power.$2  $tol2bus_power mW
    
    #************************************
    print_msg "  SMC Controller: Estimation"
    p=$(get_stat  timestamp$1.system.smccontroller.pkt_size_system.smccontroller_pipeline.master::total )
    smcctrl_power=`perl -l -e "print ($p * 8.0 / $t * $SMCCTRL_ENERGY_PER_BIT /1000.0/1000.0/1000.0)"`  #mW
    set_stat smcctrl_power.$2  $smcctrl_power mW
    
    #************************************
    print_msg "  Processor Power: Estimation"
    for (( c=0; c<$GEM5_NUMCPU; c++ )); do
        set_stat cpu$c.power.$2 $(calculate_cpu_power $1 system.cpu$c $HOST_CLOCK_FREQUENCY_GHz )
    done
    set_stat pimcpu.power.$2 $(calculate_cpu_power $1 system.pim_sys.cpu $PIM_CLOCK_FREQUENCY_GHz )
    
    #************************************
    print_msg "  Total Power ..."
    if [ $1 == 8 ];
    then
        # Power related to host:
        power_related_to_host=`perl -l -e "print (
            $(get_stat link_power.host)+
            $(get_stat dram_power.host)+
            $(get_stat vaultctrl_power.host)+
            $(get_stat l2_power.host)+
            $(get_stat cpu0.li_power.host)+
            $(get_stat cpu0.ld_power.host)+
            $(get_stat cpu1.li_power.host)+
            $(get_stat cpu1.ld_power.host)+
            $(get_stat smcxbar_power.host)+
            $(get_stat membus_power.host)+
            $(get_stat tol2bus_power.host)+
            $(get_stat smcctrl_power.host)+
            $(get_stat cpu0.power.host)+
            $(get_stat cpu1.power.host)
        )"`  #mW
        set_stat power_related_to_host $power_related_to_host mW        
    elif [ $1 == 6 ];
    then
        # Power related to host:
        power_related_to_pim=`perl -l -e "print (
            $(get_stat dram_power.pim)+
            $(get_stat vaultctrl_power.pim)+
            $(get_stat atomic_power.pim)+
            $(get_stat smcxbar_power.pim)+
            $(get_stat pimcpu.power.pim)
        )"`  #mW
        set_stat power_related_to_pim $power_related_to_pim mW      
        
    else
        print_err "Illegal timestamp! $1"
    fi
}

#######################################################################################
# $1 timestamp
# $2 cache name
# $3 cache statistics file
function calculate_cache_power
{
    acc=$(get_stat timestamp$1.$2.demand_accesses::total) # number of accesses
    T=$(get_stat timestamp$1.sim_seconds) # seconds
    leakage=$(get_value_from_file "Total leakage power of a bank" 7 ./cacti/$3) #mW
    dyn=$(get_value_from_file "Total dynamic read energy per access" 7 ./cacti/$3) #nJ
    dyn_pwr=`perl -l -e "print( $dyn*$acc/$T/1000/1000 )"` # mW
    cache_pwr=`perl -l -e "print( $dyn_pwr + $leakage )"` # mW
    echo "$cache_pwr mW  dyn: $dyn_pwr  leak: $leakage"
}

#######################################################################################
# $1 timestamp
# $2 cpu name
# $3 frequency
function calculate_cpu_power
{
    freq=$3
    # Voltage Scaling based on frequency:
    # 2GHZ --> 1.045V
    # 1GHz --> 0.76V    
    vdd=`perl -l -e "print( (0.76 + ($freq-1.0)*(1.045-0.76) ))"` # mW    
    t=$(get_stat timestamp$1.sim_seconds)
    f=$(get_stat timestamp$1.$2.idle_fraction) # idle fraction
    idle_pwr=`perl -l -e "print( ($f)    * $CPU_IDLE_POWER_AT_2GHZ * ($freq / 2.0) * ($vdd/1.045)**2 )"` # mW
    actv_pwr=`perl -l -e "print( (1.0-$f)* $CPU_ACTIVE_POWER_AT_2GHZ * ($freq / 2.0) * ($vdd/1.045)**2 )"` # mW
    pwr=`perl -l -e "print( $idle_pwr + $actv_pwr + $CPU_LEAKAGE_POWER )"` # mW
    
    if [ $2 == system.pim_sys.cpu ]; then
        pwr=`perl -l -e "print( $pwr + $PIM_PERIPHERAL_POWER )"`
    fi
    #>&2 echo "$1 $2 $3 voltage=$vdd idle_fraction=$f  power=$pwr"
    echo $pwr
}

#######################################################################################
# CACTI
function call_cacti
{
    print_msg "Running CACTI ..."
    mkdir -p cacti
    
    if ! [ -f $GEM5_BUILD_DIR/$CACTI/cacti ]; then
        print_err "CACTI not found! please rebuild the scenario with -b switch"
    fi

    _ERROR=FALSE
    if [ $HAVE_L2_CACHES == TRUE ]; then
        load_model system/cacti_l2.sh system.l2 > cacti/cacti_l2.cfg
        $GEM5_BUILD_DIR/$CACTI/cacti -infile cacti/cacti_l2.cfg > cacti/cacti_l2.txt || _ERROR=TRUE
    fi
    
    if [ $HAVE_L1_CACHES == TRUE ]; then
        load_model system/cacti_l1.sh system.cpu0.dcache > cacti/cacti_ld.cfg
        $GEM5_BUILD_DIR/$CACTI/cacti -infile cacti/cacti_ld.cfg > cacti/cacti_ld.txt || _ERROR=TRUE
        
        load_model system/cacti_l1.sh system.cpu0.icache > cacti/cacti_li.cfg
        $GEM5_BUILD_DIR/$CACTI/cacti -infile cacti/cacti_li.cfg > cacti/cacti_li.txt || _ERROR=TRUE
    fi
    
    if [ $_ERROR == TRUE ]; then
        print_err "Error happened in call to CACTI!"    
        cat cacti/cacti_l2.txt;
        cat cacti/cacti_ld.txt;
        cat cacti/cacti_li.txt;
    fi
}

#######################################################################################
# Alias a single gem5 stat
function alias_gem5_stat
{
    stat=$1
    alias=$2    #$(extend $2 20 _);
    S=$(grep $stat ./stats_gem5.txt);
    S=${S/$stat/$alias}
    S="$S [${stat/./_}]"
    echo -e $S >> ./stats.txt

}

#######################################################################################
# Get the value of one stat from stats.txt
function get_stat
{
    S=$(grep "$1" $M5_OUTDIR/stats.txt);
    L=( $S )
    Ret=${L[1]}
    if [ -z $Ret ]; then
        >&2 echo "Warning: stat $1 was not found, we assume it to be zero!"
        Ret=0
    fi
    echo $Ret
}

#######################################################################################
# Get the value of one parameter from params.txt
function get_param
{
    S=$(grep "$1" $M5_OUTDIR/params.txt);
    L=( $S )
    echo ${L[1]}
}

#######################################################################################
# Get one value from one file
# $1: stat
# $2: column
# $3: file name
function get_value_from_file
{
    S=$(grep "$1" $3);
    L=( $S )
    echo ${L[$2]}
}

#######################################################################################
# Create a new stat
# $1 = name
# $2 = value
# $3 = unit
function set_stat
{
    echo -e "$1\t$2\t$3\t$4\t$5\t$6\t$7\t$8" >> $M5_OUTDIR/stats.txt;
}

#######################################################################################
# Rename gem5 statistics to make them more understandable
function alias_gem5_statistics
{
    print_msg "Creating alias for gem5 stats ..."
    _PWD=${PWD}
    cd $SCENARIO_CASE_DIR/m5out/
    cp ./stats_gem5.txt ./stats.txt
    
    if ! [ $GEM5_MACHINETYPE == NONE ]; then
        echo "---------- Aliases ------------------------------" >> stats.txt
        
        # Here are the aliases
        alias_gem5_stat "timestamp2.sim_ticks"    "sim_ticks.offload_kernel"
        alias_gem5_stat "timestamp4.sim_ticks"    "sim_ticks.offload_task"
        alias_gem5_stat "timestamp6.sim_ticks"    "sim_ticks.pim"
        alias_gem5_stat "timestamp8.sim_ticks"    "sim_ticks.host"
        ratio=`perl -l -e "print ($(get_stat sim_ticks.host) / $(get_stat sim_ticks.pim))"`
        set_stat sim_ticks.ratio_host_to_pim $ratio ratio
    fi
    
    if ! [ $ARCH == "NONE" ]; then
        gather_software_stats
    fi
    

#     hit_rate=$(get_stat timestamp6.system.pim_sys.dtlb.tlb_dyn_hit_rate)
#     if [ ${hit_rate%.*} -gt 90 ]; then
#         print_msg "PIM TLB Hit Rate: $hit_rate"
#     else
#         print_war "TLB Hit Rate is low: $hit_rate%"
#     fi
    
    returntopwd
}

# This function is only for the PIM Cluster simulation
# allocate a region from the start to the end
#######################################################################################
# $1 = Size  (B)
function allocate_spm_region()
{
    if [ -f SPM_RESERVED_POINTER ]; then
        SPM_RESERVED_POINTER=$(cat SPM_RESERVED_POINTER)
    else
        SPM_RESERVED_POINTER=$SPM_RESERVED_START
    fi
    
    echo $SPM_RESERVED_POINTER
    # Advance the pointer
    SPM_RESERVED_POINTER=0x$(printf "%X" $(($SPM_RESERVED_POINTER + $1)))
    hex_compare $SPM_RESERVED_POINTER ">" $SPM_RESERVED_END "Reserved memory allocation in the SPM failed!"
    echo $SPM_RESERVED_POINTER > SPM_RESERVED_POINTER
}

