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
TO="1h"
CONF=0.9  # Confidence coefficient
PREC=0.2  # Relative precision
SPLITS=(2 3 6 11)  # RESTART splittings to test
MEAN_FAILURE_TIMES=(2000 8000 32000 128000)
NDC=6  # Number of disk clusters
NCT=2  # Number of controller types
NPT=2  # Number of processor types
RED=2  # System redundancy
EXPNAME="database"
#
show "Configuring experiments for $NDC disk clusters,"
show "                            $NCT controller types,"
show "                            $NPT processor types,"
show "                            $RED redundancy"
SPLITTING="--splitting "
for S in "${SPLITS[@]}"; do SPLITTING+="$S,"; done; SPLITTING="${SPLITTING%,}"
STOP_CRITERION="--stop-conf $CONF $PREC"
ETIMEOUT="${TO##*[0-9]}"  # Timeout per experiment (one ifun, all splits)
ETIMEOUT=$(bc <<< "${TO%%[a-z]*}*${#SPLITS[@]}*2")"$ETIMEOUT"
show "Timeouts: $TO per split; $ETIMEOUT per experiment"
MIN_OC=$(min_num_oc $RED $NDC $NCT $NPT)
COMP_FUN_ADD="\"+\""
COMP_FUN_COA=$(comp_fun_coarse $RED $NDC $NCT $NPT)  # Coarse
COMP_FUN_MED=$(comp_fun_med    $RED $NDC $NCT $NPT)  # Medium
COMP_FUN_FIN=$(comp_fun_fine   $RED $NDC $NCT $NPT)  # Fine
STANDARD_MC="--flat -e nosplit $STOP_CRITERION --timeout $TO"
RESTART_ADHOC="--adhoc $MIN_OC $STOP_CRITERION $SPLITTING --timeout $TO"
RESTART_ACOMP1="--acomp $COMP_FUN_ADD $STOP_CRITERION $SPLITTING --timeout $TO"
RESTART_ACOMP2="--acomp $COMP_FUN_COA $STOP_CRITERION $SPLITTING --timeout $TO --post-process exp 2"
RESTART_ACOMP3="--acomp $COMP_FUN_MED $STOP_CRITERION $SPLITTING --timeout $TO --post-process exp 2"
RESTART_ACOMP4="--acomp $COMP_FUN_FIN $STOP_CRITERION $SPLITTING --timeout $TO --post-process exp 2"


# Launch experiments
show "Launching experiments:"
for mft in "${MEAN_FAILURE_TIMES[@]}"
do
	show -n "  · for mean failure times ~ $mft..."

	# Generate model and properties files to fit this experiment
	MODEL_FILE=database_mft${mft}.sa
	PROPS_FILE=database_mft${mft}.pp
	LOG=${RESULTS}/database_mft${mft}
	bash $EXP_GEN 2 6 2 2 $mft 1>$MODEL_FILE 2>$PROPS_FILE
	EXE=`/bin/echo -e "timeout -s 15 $ETIMEOUT ./fig $MODEL_FILE $PROPS_FILE"`

	# RESTART with monolithic (auto ifun) experiments are omitted
	# since the importance vector wouldn't fit in memory

	# RESTART with compositional (auto ifun), version 1
	poll_till_free $EXPNAME; show -n " AC1"
	$EXE $RESTART_ACOMP1 1>>${LOG}"_AC1.out" 2>>${LOG}"_AC1.err" &

	# RESTART with compositional (auto ifun), version 2
	poll_till_free $EXPNAME; show -n ", AC2"
	$EXE $RESTART_ACOMP2 1>>${LOG}"_AC2.out" 2>>${LOG}"_AC2.err" &

	# RESTART with compositional (auto ifun), version 3
	poll_till_free $EXPNAME; show -n ", AC3"
	$EXE $RESTART_ACOMP3 1>>${LOG}"_AC3.out" 2>>${LOG}"_AC3.err" &

	# RESTART with compositional (auto ifun), version 4
	poll_till_free $EXPNAME; show -n ", AC4"
	$EXE $RESTART_ACOMP4 1>>${LOG}"_AC4.out" 2>>${LOG}"_AC4.err" &

	# RESTART with ad hoc
	poll_till_free $EXPNAME; show -n ", AH"
	$EXE $RESTART_ADHOC 1>>${LOG}"_AH.out" 2>>${LOG}"_AH.err" &
  
	# Standard Monte Carlo
	poll_till_free $EXPNAME; show -n ", MC"
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
EXPERIMENTS=("${MEAN_FAILURE_TIMES[@]/#/mft}")
build_table "est"  $RESULTS EXPERIMENTS[@] IFUNS[@] SPLITS[@] $CONF $PREC \
	&> $RESULTS/table_estimates.txt
build_table "time" $RESULTS EXPERIMENTS[@] IFUNS[@] SPLITS[@] $CONF $PREC \
	&> $RESULTS/table_times.txt
show " done"


# Turn lights off
EXE_WTIME=$(format_seconds $SECONDS)  
show "Script execution walltime was $EXE_WTIME"
show "Results are in ${RESULTS}"
exit 0

