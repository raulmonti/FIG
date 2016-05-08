#!/bin/bash
#
# Author:  Carlos E. Budde
# Date:    22.03.2016
# License: GPLv3
#

set -e
ECHO=`echo "/bin/echo -e"`
THIS_DIR=`readlink -f "$(dirname ${BASH_SOURCE[0]})"`


# Probe resources
source "$THIS_DIR/../../fig_utils.sh" || \
	($ECHO "[ERROR] Couldn't find fig_utils.sh" && exit 1)
if [[ "$(type -t build_fig)" != "function" ]]
then
	$ECHO "[ERROR] Bash function \"build_fig\" is undefined"
	exit 1;
elif [[ "$(type -t copy_model_file)" != "function" ]]
then
	$ECHO "[ERROR] Bash function \"copy_model_file\" is undefined"
	exit 1;
fi


# Build project
$ECHO "Building FIG"
build_fig $THIS_DIR
if [ ! -f ./fig ]; then $ECHO "[ERROR] Something went wrong"; exit 1; fi


# Prepare experiment's directory and files
$ECHO "Preparing experiments environment:"
MODEL_FILE="tandem_queue.sa"
copy_model_file $MODEL_FILE $THIS_DIR && \
	$ECHO "  · using model file $MODEL_FILE"
PROPS_FILE="tandem_queue.pp"
$ECHO 'P( q2 > 0 U lost )' > $PROPS_FILE && \
	$ECHO "  · using properties file $PROPS_FILE"
N=0; RESULTS="results_$N"
while [ -d $RESULTS ]; do N=$((N+1)); RESULTS="results_$N"; done
mkdir $RESULTS && unset N && \
	$ECHO "  · results will be stored in subdir \"${RESULTS}\""


# Experiments configuration
$ECHO "Configuring experiments"
declare -a QUEUES_CAPACITIES=(8 10 12 14)
STOP_CRITERION="--stop-conf 0.90 0.2"  # Confidence coeff. and rel. precision
SPLITTINGS="--splitting 2,3,6"         # Splitting values for RESTART engine
STANDARD_MC="-e nosplit --flat $STOP_CRITERION"
RESTART_ADHOC="--adhoc q2 $STOP_CRITERION $SPLITTINGS"
RESTART_AUTO_COUPLED="--auto-coupled $STOP_CRITERION $SPLITTINGS"
RESTART_AUTO_SPLIT="--auto-split \"+\" $STOP_CRITERION $SPLITTINGS"


# Launch experiments
$ECHO "Launching experiments:"
for c in "${QUEUES_CAPACITIES[@]}"
do
	$ECHO -n "  · for queues capacity = $c..."

	# Modify model file to fit this experiment
	MODEL_FILE_C=${MODEL_FILE%.sa}"_${c}.sa"
	BLANK="[[:space:]]*"
	C_DEF="^const${BLANK}int${BLANK}c${BLANK}=${BLANK}[_\-\+[:alnum:]]*;"
	sed -e "s/${C_DEF}/const int c = $c;/1" $MODEL_FILE > $MODEL_FILE_C
	LOG=${RESULTS}/tandem_queue_c${c}
	EXE=`$ECHO "timeout -s 15 18h ./fig $MODEL_FILE_C $PROPS_FILE"`

	# Standard Monte Carlo
	poll_till_free; $ECHO -n " MC"
	$EXE $STANDARD_MC 1>>${LOG}"_MC.out" 2>>${LOG}"_MC.err" &

	# RESTART with ad hoc
	poll_till_free; $ECHO -n ", AH"
	$EXE $RESTART_ADHOC 1>>${LOG}"_AH.out" 2>>${LOG}"_AH.err" &

	# RESTART with auto (coupled)
	poll_till_free; $ECHO -n ", AC"
	$EXE $RESTART_AUTO_COUPLED 1>>${LOG}"_AC.out" 2>>${LOG}"_AC.err" &

	# RESTART with auto (split)
	poll_till_free; $ECHO -n ", AS"
	$EXE $RESTART_AUTO_SPLIT 1>>${LOG}"_AS.out" 2>>${LOG}"_AS.err"  &

	$ECHO "... done"
done


# Wait till termination
$ECHO "Waiting for all experiments to finish"
declare -a A=(. :) ; N=0
RUNNING=`$ECHO "pgrep -u $(whoami) fig"`
while [ -n "`$RUNNING`" ]; do $ECHO -n ${A[$N]}; N=$(((N+1)%2)); sleep 2m; done
unset A N
$ECHO
$ECHO "All experiments finished"


exit 0

