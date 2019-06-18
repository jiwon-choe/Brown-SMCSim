#!/bin/bash
# Automated simulation of gem5 without the need for attaching a terminal or taking checkpoints

if ! [ -z $LIST_PREVIOUS_RESULTS ]; then return; fi;

##### homo: homogeneous cases:
#   In this type of scenario, hardware is not changed across different cases, so we can use one
#   one checkpoint for all the cases
##### hetero: heterogenous cases:
#   In this type of scenario, hardware parameters change across different cases. For example 
#   the clock frequency or buffer size of a component may change. So for each case we must take
#   a new checkpoint

if [ $1 == "homo" ]; then
	export AUTOMATED_CHECKPOINT_DIR=$SMC_WORK_DIR/checkpoints/$__SCENARIO/
elif [ $1 == "hetero" ]; then
	export AUTOMATED_CHECKPOINT_DIR=$SMC_WORK_DIR/checkpoints/$__SCENARIO/$__CASE/
else
	print_err "gem5_automated_sim.sh needs an argument: [homo or hetero]";
	print_msg "homo: all scenario cases will share the same checkpoint, because the architecture is not changed across different cases"
	print_msg "hetero: each case will have its own checkpoint, because of the change in the architectural params (e.g. NUMCPU)"
fi
export GEM5_AUTOMATED_SIMULATION_MODE=$1

if [ -d $AUTOMATED_CHECKPOINT_DIR ]; then
	print_msg "Resuming checkpoint from: $AUTOMATED_CHECKPOINT_DIR"
	export GEM5_CHECKPOINT_RESTORE=TRUE
	export GEM5_CHECKPOINT_LOCATION=$(ls -d $AUTOMATED_CHECKPOINT_DIR/*/);
	export GEM5_AUTOMATED_SCRIPT=$M5_OUTDIR/do.rcs
else
	export GEM5_CHECKPOINT_RESTORE=FALSE
	print_msg "Checkpoint not found! taking a checkpoint for the first time ..."
	print_msg "Switching to atomic mode, for faster checkpointing ..."
	export GEM5_AUTOMATED_SCRIPT=$SMC_UTILS_DIR/models/gem5_automated_boot.rcS
	export GEM5_CPUTYPE=atomic
fi

# Create the linux boot script
export GEM5_AUTOMATED_SIMULATION=TRUE

if [ -z $SCENARIO_CASE_DIR ]; then
	print_err "\$SCENARIO_CASE_DIR is undefined! $SCENARIO_CASE_DIR"
	exit
else

echo -e "#!/bin/sh
if [ -f /flag_execute2 ]; then
	echo \"We should exit!!!\"
	exit 0
fi
echo \"########################################\"
echo \"Automated run script by Erfan Azarkhish\"
echo \"########################################\"
touch /flag_execute2
cd /
./get
cd work/
echo \"##############JOB START#################\"
./do
echo \"##############JOB END###################\"
/sbin/m5 exit
	" > $M5_OUTDIR/do.rcs
fi

