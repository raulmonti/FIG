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
while [ -d $RESULTS ]; do N=`expr $N + 1`; RESULTS="results_$N"; done
mkdir $RESULTS && unset N && \
	$ECHO "  · results will be stored in subdir \"${RESULTS}\""


# Experiments configuration
$ECHO "Configuring experiments"
declare -a QUEUES_CAPACITIES=(8 10 12 14)
STOP_CRITERION="0.95 0.2"  # Confidence coefficient and relative precision
SPLITTINGS="2 5 11"        # Splitting values to test with the RESTART engine
THRESHOLDS="ams"           # Thresholds building technique
EXE=`$ECHO "./fig $MODEL_FILE $PROPS_FILE"`
STANDARD_MC="nosplit algebraic flat"
RESTART_ADHOC="restart algebraic adhoc \"q2\""
RESTART_AUTO_COUPLED="restart concrete_coupled auto"
RESTART_AUTO_SPLIT="restart concrete_split auto \"+\""
STANDARD_MC+=" $THRESHOLDS $STOP_CRITERION"
RESTART_ADHOC+=" $THRESHOLDS $STOP_CRITERION"
RESTART_AUTO_COUPLED+=" $THRESHOLDS $STOP_CRITERION"
RESTART_AUTO_SPLIT+=" $THRESHOLDS $STOP_CRITERION"


# Launch experiments
$ECHO "Launching experiments:"
for c in "${QUEUES_CAPACITIES[@]}"
do
	$ECHO -n "  · for queues capacity = $c..."

	# Modify model file to this experiment's size
	BLANK="[[:space:]]*"
	C_DEF="^const${BLANK}int${BLANK}c${BLANK}=${BLANK}[_-+[:alnum:]]*;"
	sed -i -e "s/${C_DEF}/const int c = $c;/1" $MODEL_FILE
	LOGout=${RESULTS}/tandem_queue_c${c}.out
	LOGerr=${RESULTS}/tandem_queue_c${c}.err

	# Standard Monte Carlo
	poll_till_free
	$ECHO -n " MC"
	$EXE $STANDARD_MC "0" 1>>${LOGout%.out}"_MC.out" \
	                      2>>${LOGerr%.err}"_MC.err" &
	# RESTART with ad hoc
	poll_till_free
	$ECHO -n ", AH"
	$EXE $RESTART_ADHOC "$SPLITTINGS" 1>>${LOGout%.out}"_AH.out" \
	                                  2>>${LOGerr%.err}"_AH.err" &
	# RESTART with auto (coupled)
	poll_till_free
	$ECHO -n ", AC"
	$EXE $RESTART_AUTO_COUPLED "$SPLITTINGS" 1>>${LOGout%.out}"_AC.out" \
	                                         2>>${LOGerr%.err}"_AC.err" &
	# RESTART with auto (split)
	poll_till_free
	$ECHO -n ", AS"
	$EXE $RESTART_AUTO_SPLIT "$SPLITTINGS" 1>>${LOGout%.out}"_AS.out"  \
	                                       2>>${LOGerr%.err}"_AS.err"  &
	$ECHO "... done"
done


# Wait till termination
$ECHO "Waiting for all experiments to finish"
declare -a A=(. :) ; N=0
RUNNING=`$ECHO "pgrep -u $(whoami) fig"`
while [ -n "`$RUNNING`" ]; do $ECHO -n ${A[$N]}; N=$((($N+1)%2)); sleep 9; done
unset A N
$ECHO
$ECHO "All experiments finished"


exit 0

