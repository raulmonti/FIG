#!/bin/bash
#
# Author:  Carlos E. Budde
# Date:    22.04.2016
# License: GPLv3
#

set -e
show(){ /bin/echo -e "$@"; }
CWD=`readlink -f "$(dirname ${BASH_SOURCE[0]})"`


# Probe resources
source "$CWD/../../fig_utils.sh" || \
	(show "[ERROR] Couldn't find fig_utils.sh" && exit 1)
if [[ "$(type -t build_fig)" != "function" ]]
then
	show "[ERROR] Bash function \"build_fig\" is undefined"
	exit 1;
elif [[ "$(type -t copy_model_file)" != "function" ]]
then
	show "[ERROR] Bash function \"copy_model_file\" is undefined"
	exit 1;
fi
source "$CWD/../ifuns.sh" || \
	(show "[ERROR] Couldn't find ifuns.sh" && exit 1)
if [[ "$(type -t min_num_oc)" != "function" ]]
then
	show "[ERROR] Bash function \"min_num_oc\" is undefined"
	exit 1;
elif [[ "$(type -t comp_fun_coarse)" != "function" ]]
then
	show "[ERROR] Bash function \"comp_fun_coarse\" is undefined"
	exit 1;
elif [[ "$(type -t comp_fun_med)" != "function" ]]
then
	show "[ERROR] Bash function \"comp_fun_med\" is undefined"
	exit 1;
elif [[ "$(type -t comp_fun_fine)" != "function" ]]
then
	show "[ERROR] Bash function \"comp_fun_fine\" is undefined"
	exit 1;
fi


# Build project
show "Building FIG"
build_fig $CWD
if [ ! -f ./fig ]; then show "[ERROR] Something went wrong"; exit 1; fi


# Prepare experiment's directory and files
show "Preparing experiments environment:"
EXP_GEN="database-gen.sh"
copy_model_file $EXP_GEN $CWD && \
	show "  · using model&properties generator \"$EXP_GEN\""
N=0; RESULTS="results_$N"
while [ -d $RESULTS ]; do N=$((N+1)); RESULTS="results_$N"; done
mkdir $RESULTS && unset N && \
	show "  · results will be stored in subdir \"${RESULTS}\""


# Experiments configuration
TO="4h"
CONF=0.9  # Confidence coefficient
PREC=0.3  # Relative precision
SPLITS=(2 3 6 11)  # RESTART splittings to test
#MEAN_FAILURE_TIMES=(2000 8000 32000 128000)
MEAN_FAILURE_TIMES=(500 1000 2000 4000 8000)
NDC=6  # Number of disk clusters
NCT=2  # Number of controller types
NPT=2  # Number of processor types
RED=3  # System redundancy (original: 2)
EXPNAME="database"
#
show "Configuring experiments for $NDC disk clusters,"
show "                            $NCT controller types,"
show "                            $NPT processor types,"
show "                            $RED redundancy"
STOP_CRITERION="--stop-conf $CONF $PREC"
ETIMEOUT="${TO##*[0-9]}"  # Experiment timeout (ifun&thr building + sim)
ETIMEOUT=$(bc -l <<< "scale=0; ${TO%%[a-z]*}*1.4/1")"$ETIMEOUT"
show "Timeouts: $TO per simulation; $ETIMEOUT per experiment"
MIN_OC=$(min_num_oc $RED $NDC $NCT $NPT)
COMP_FUN_ADD="\"+\""
COMP_FUN_COA=$(comp_fun_coarse $RED $NDC $NCT $NPT)  # Coarse
COMP_FUN_MED=$(comp_fun_med    $RED $NDC $NCT $NPT)  # Medium
COMP_FUN_FIN=$(comp_fun_fine   $RED $NDC $NCT $NPT)  # Fine
STANDARD_MC="--flat -e nosplit $STOP_CRITERION --timeout $TO"
RESTART_ADHOC="--adhoc $MIN_OC $STOP_CRITERION --timeout $TO"
RESTART_ACOMP1="--acomp $COMP_FUN_ADD $STOP_CRITERION --timeout $TO"
RESTART_ACOMP2="--acomp $COMP_FUN_COA $STOP_CRITERION --timeout $TO --post-process exp 2"
RESTART_ACOMP3="--acomp $COMP_FUN_MED $STOP_CRITERION --timeout $TO --post-process exp 2"
RESTART_ACOMP4="--acomp $COMP_FUN_FIN $STOP_CRITERION --timeout $TO --post-process exp 2"


# Launch experiments
show "Launching experiments:"
for mft in "${MEAN_FAILURE_TIMES[@]}"
do
	show -n "  · for mean failure times ~ $mft..."

	# Generate model and properties files to fit this experiment
	MODEL_FILE=${EXPNAME}_mft${mft}.sa
	PROPS_FILE=${EXPNAME}_mft${mft}.pp
	LOG=${RESULTS}/${EXPNAME}_mft${mft}
	bash $EXP_GEN $RED $NDC $NCT $NPT $mft 1>$MODEL_FILE 2>$PROPS_FILE
	EXE=`/bin/echo -e "timeout -s 15 $ETIMEOUT ./fig $MODEL_FILE $PROPS_FILE"`

	# Launch a job for each splitting value (improves load balance)
	for s in "${SPLITS[@]}"
	do
		# RESTART with monolithic (auto ifun) experiments are omitted
		# since the importance vector wouldn't fit in memory

		# RESTART with compositional (auto ifun), version 1
		poll_till_free $EXPNAME; show -n " AC1_s${s},"
		$EXE $RESTART_ACOMP1 -s $s 1>>${LOG}"_AC1_s${s}.out" \
		                           2>>${LOG}"_AC1_s${s}.err" &
	
		# RESTART with compositional (auto ifun), version 2
		poll_till_free $EXPNAME; show -n " AC2_s${s},"
		$EXE $RESTART_ACOMP2 -s $s 1>>${LOG}"_AC2_s${s}.out" \
		                           2>>${LOG}"_AC2_s${s}.err" &
	
		# RESTART with compositional (auto ifun), version 3
		poll_till_free $EXPNAME; show -n " AC3_s${s},"
		$EXE $RESTART_ACOMP3 -s $s 1>>${LOG}"_AC3_s${s}.out" \
		                           2>>${LOG}"_AC3_s${s}.err" &
	
		# RESTART with compositional (auto ifun), version 4
		poll_till_free $EXPNAME; show -n " AC4_s${s},"
		$EXE $RESTART_ACOMP4 -s $s 1>>${LOG}"_AC4_s${s}.out" \
		                           2>>${LOG}"_AC4_s${s}.err" &
	
		# RESTART with ad hoc
		poll_till_free $EXPNAME; show -n " AH_s${s},"
		$EXE $RESTART_ADHOC -s $s 1>>${LOG}"_AH_s${s}.out" \
		                          2>>${LOG}"_AH_s${s}.err" &
	done
  
	# Standard Monte Carlo
	poll_till_free $EXPNAME; show -n " MC"
	$EXE $STANDARD_MC 1>>${LOG}"_MC.out" 2>>${LOG}"_MC.err" &

	show "... done"
done


# Wait till termination, making sure everything dies after the timeout
show -n "Waiting for all experiments to finish..."
`PIDS=$(ps -fC "fig" | grep $EXPNAME | awk '{ print $2 }') \
 sleep $ETIMEOUT; kill -15 $PIDS &>/dev/null;              \
 sleep 2;         kill  -9 $PIDS &>/dev/null`              &
disown %%; wait &>/dev/null; killall sleep &>/dev/null
show " done"


# Build summary charts
show -n "Building tables..."
IFUNS=("MC" "AH" "AC1" "AC2" "AC3" "AC4")
for mft in "${MEAN_FAILURE_TIMES[@]}"; do
	# Unify each importance function results in a single file
	LOG=${RESULTS}/${EXPNAME}_mft${mft}
	for IFUN in "${IFUNS[@]}"; do
		if [[ ${IFUN} == "MC" ]]; then continue; fi
		cat ${LOG}_${IFUN}_s[0-9]*.out >> ${LOG}"_${IFUN}.out"
		cat ${LOG}_${IFUN}_s[0-9]*.err >> ${LOG}"_${IFUN}.err"
		rm  ${LOG}_${IFUN}_s[0-9]*.{out,err}
	done
done
EXPERIMENTS=("${MEAN_FAILURE_TIMES[@]/#/mft}")
build_c_table "est"  $RESULTS EXPERIMENTS[@] IFUNS[@] SPLITS[@] $CONF $PREC \
	&> $RESULTS/table_estimates.txt
build_c_table "prec" $RESULTS EXPERIMENTS[@] IFUNS[@] SPLITS[@] $CONF $PREC \
	&> $RESULTS/table_precisions.txt
build_c_table "time" $RESULTS EXPERIMENTS[@] IFUNS[@] SPLITS[@] $CONF $PREC \
	&> $RESULTS/table_times.txt
show " done"


# Turn lights off
EXE_WTIME=$(format_seconds $SECONDS)  
show "Finished on $(date)"
show "Script execution walltime was $EXE_WTIME"
show "Results are in ${RESULTS}"
exit 0

