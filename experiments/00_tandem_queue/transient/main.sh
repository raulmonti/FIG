#!/bin/bash
#
# Author:  Carlos E. Budde
# Date:    22.03.2016
# License: GPLv3
#

set -e
show(){ /bin/echo -e "$@"; }
THIS_DIR=`readlink -f "$(dirname ${BASH_SOURCE[0]})"`


# Probe resources
source "$THIS_DIR/../../fig_utils.sh" || \
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


# Build project
show "Building FIG"
build_fig $THIS_DIR
if [ ! -f ./fig ]; then show "[ERROR] Something went wrong"; exit 1; fi


# Prepare experiment's directory and files
show "Preparing experiments environment:"
MODEL_FILE="tandem_queue.sa"
copy_model_file $MODEL_FILE $THIS_DIR && \
	show "  · using model file $MODEL_FILE"
PROPS_FILE="tandem_queue.pp"
echo 'P( q2 > 0 U lost )' > $PROPS_FILE && \
	show "  · using properties file $PROPS_FILE"
N=0; RESULTS="results_$N"
while [ -d $RESULTS ]; do N=$((N+1)); RESULTS="results_$N"; done
mkdir $RESULTS && unset N && \
	show "  · results will be stored in subdir \"${RESULTS}\""


# Experiments configuration
#TO="10h"
TO="1m"
#CONF=0.9  # Confidence coefficient
#PREC=0.2  # Relative precision
CONF=0.6  # Confidence coefficient
PREC=0.6  # Relative precision
#SPLITS=(2 3 6)  # RESTART splittings to test
SPLITS=(3 5 7)  # RESTART splittings to test
#QUEUES_CAPACITIES=(8 10 12 14)
QUEUES_CAPACITIES=(9)
EXPNAME="tandem_queue"
#
show "Configuring experiments"
SPLITTING="--splitting "
for S in "${SPLITS[@]}"; do SPLITTING+="$S,"; done; SPLITTING="${SPLITTING%,}"
STOP_CRITERION="--stop-conf $CONF $PREC"
ETIMEOUT="${TO##*[0-9]}"  # Timeout per experiment (one ifun, all splits)
ETIMEOUT=$(bc <<< "${TO%%[a-z]*}*${#SPLITS[@]}*2")"$ETIMEOUT"
show "Timeouts: $TO per split; $ETIMEOUT per experiment"
STANDARD_MC="-e nosplit --flat $STOP_CRITERION --timeout $TO"
RESTART_ADHOC="--adhoc q2 $STOP_CRITERION $SPLITTING --timeout $TO"
RESTART_AMONO="--amono $STOP_CRITERION $SPLITTING --timeout $TO"
RESTART_ACOMP="--acomp \"+\" $STOP_CRITERION $SPLITTING --timeout $TO"


# Launch experiments
show "Launching experiments:"
for c in "${QUEUES_CAPACITIES[@]}"
do
	show -n "  · for queues capacity = $c..."

	# Modify model file to fit this experiment
	MODEL_FILE_C=${MODEL_FILE%.sa}"_${c}.sa"
	BLANK="[[:space:]]*"
	C_DEF="^const${BLANK}int${BLANK}c${BLANK}=${BLANK}[_\-\+[:alnum:]]*;"
	sed -e "s/${C_DEF}/const int c = $c;/1" $MODEL_FILE > $MODEL_FILE_C
	LOG=${RESULTS}/tandem_queue_c${c}
	EXE=`/bin/echo -e "timeout -s 15 $ETIMEOUT ./fig $MODEL_FILE_C $PROPS_FILE"`

	# RESTART with monolithic (auto ifun)
	poll_till_free $EXPNAME; show -n " AM"
	$EXE $RESTART_AMONO 1>>${LOG}"_AM.out" 2>>${LOG}"_AM.err" &

	# RESTART with compositional (auto ifun)
	poll_till_free $EXPNAME; show -n ", AC"
	$EXE $RESTART_ACOMP 1>>${LOG}"_AC.out" 2>>${LOG}"_AC.err" &

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
PIDS=$(ps -fC "fig" | grep $EXPNAME | awk '{ print $2 }')
`sleep $ETIMEOUT; kill -9 $PIDS &>/dev/null;` &
wait &>/dev/null
show " done"


# Build summary charts
show -n "Building tables..."
IFUNS=("MC" "AH" "AC" "AM")
EXPERIMENTS=("${QUEUES_CAPACITIES[@]}")
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

