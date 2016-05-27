#!/bin/bash
#
# Author:  Carlos E. Budde
# Date:	22.03.2016
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


# Build project
show "Building FIG"
build_fig $CWD
if [ ! -f ./fig ]; then show "[ERROR] Something went wrong"; exit 1; fi


# Prepare experiment's directory and files
show "Preparing experiments environment:"
MODEL_FILE="queues_with_breakdowns.sa"
copy_model_file $MODEL_FILE $CWD && \
	show "  · using model file $MODEL_FILE"
PROPS_FILE="queues_with_breakdowns.pp"
echo 'P( !reset U lost )' > $PROPS_FILE && \
	show "  · using properties file $PROPS_FILE"
N=0; RESULTS="results_$N"
while [ -d $RESULTS ]; do N=$((N+1)); RESULTS="results_$N"; done
mkdir $RESULTS && unset N && \
	show "  · results will be stored in subdir \"${RESULTS}\""


# Experiments configuration
TO="16h"
CONF=0.9  # Confidence coefficient
PREC=0.2  # Relative precision
SPLITS=(2 3 6)  # RESTART splittings to test
BUFFER_CAPACITIES=(20 40 80 160)
show "Configuring experiments"
EXPNAME="queues_with_breakdowns"
SPLITTING="--splitting "
for S in "${SPLITS[@]}"; do SPLITTING+="$S,"; done; SPLITTING="${SPLITTING%,}"
STOP_CRITERION="--stop-conf $CONF $PREC"
STANDARD_MC="-e nosplit --flat $STOP_CRITERION"
RESTART_ADHOC="--adhoc buf $STOP_CRITERION $SPLITTING"
RESTART_AMONO="--amono $STOP_CRITERION $SPLITTING"
RESTART_ACOMP="--acomp \"+\" $STOP_CRITERION $SPLITTING"


# Launch experiments
show "Launching experiments:"
for k in "${BUFFER_CAPACITIES[@]}"
do
	show -n "  · for buffer capacity = $k..."

	# Modify model file to fit this experiment
	MODEL_FILE_K=${MODEL_FILE%.sa}"_${k}.sa"
	BLANK="[[:space:]]*"
	K_DEF="^const${BLANK}int${BLANK}K${BLANK}=${BLANK}[_\-\+[:alnum:]]*;"
	sed -e "s/${K_DEF}/const int K = $k;/1" $MODEL_FILE > $MODEL_FILE_K
	LOG=${RESULTS}/queues_with_breakdowns_k${k}
	EXE=`/bin/echo -e "timeout -s 15 $TO ./fig $MODEL_FILE_K $PROPS_FILE"`

	# RESTART with with monolithic (auto ifun)
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


# Wait till termination, making sure everything dies after $TO
show -n "Waiting for all experiments to finish..."
PIDS=$(ps -fC "fig" | grep $EXPNAME | awk '{ print $2 }')
`sleep $TO; kill -9 $PIDS &>/dev/null;` &> /dev/null &
wait
show " done"


# Build summary charts
show -n "Building tables..."
IFUNS=("MC" "AH" "AC" "AM")
EXPERIMENTS=("${BUFFER_CAPACITIES[@]}")
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

