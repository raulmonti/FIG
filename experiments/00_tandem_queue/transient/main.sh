#!/bin/bash
#
# Author:  Carlos E. Budde
# Date:    22.03.2016
# License: GPLv3
#

set -e
source "../../fig_utils.sh" || \
	(echo "[ERROR] Couldn't find fig_utils.sh" && exit 1)

# Build project
echo "Building FIG"
BUILD_DIR=`readlink -m ./bin`
build_fig $BUILD_DIR
FIG="$BUILD_DIR/fig/fig"

# Prepare testing environment
echo "Preparing testing environment"
MODEL_FILE="tandem_queue.sa"
copy_model_file $MODEL_FILE && \
	echo "\t· using model file $MODEL_FILE"
PROPS_FILE="tandem_queue.pp"
echo 'P( q2 > 0 U lost)' > $PROPS_FILE && \
	echo "\t· using properties file $PROPS_FILE"
N=0; RESULTS="results_$N"
while [ -d $RESULTS ]; do N=`expr $N + 1`; RESULTS="results_$N"; done
mkdir $RESULTS && unset N && \
	echo "\t· results will be stored in subdir \"${RESULTS}\""

# Experiments configuration
echo "Configuring experiments"
declare -a QUEUES_CAPACITIES=(8 10 12 14)
STOP_CRITERION="0.95 0.2"  # Confidence coefficient and relative precision
SPLITTINGS="2 3 5 9"       # Splitting values to test with the RESTART engine
THRESHOLDS="ams"           # Thresholds building technique
STANDARD_MC="nosplit algebraic flat $THRESHOLDS"
RESTART_ADHOC="restart algebraic adhoc $THRESHOLDS"
RESTART_AUTO_COUPLED="restart concrete_coupled auto $THRESHOLDS"
RESTART_AUTO_SPLIT="restart concrete_split auto $THRESHOLDS"
EXE=`echo $FIG $MODEL_FILE $PROPS_FILE`

# Launch experiments
echo "Launching experiments:"
for c in "${QUEUES_CAPACITIES[@]}"
do
	echo -n "\t· for queues capacity = $c..."

	# Modify model file to this experiment's size
	BLANK="[[:space:]]*"
	C_DEF="^const${BLANK}int${BLANK}c${BLANK}=${BLANK}[_-+[:alnum:]]*;"
	sed -i -e "s/${C_DEF}/const int c = $c;/1" $MODEL_FILE
	LOGout=${RESULTS}/tandem_queue_c${c}.out
	LOGerr=${RESULTS}/tandem_queue_c${c}.err

	# Standard Monte Carlo
	poll_till_free
	$EXE $STANDARD_MC $STOP_CRITERION "0" \
		1>>${LOGout%.out}"_MC.out"        \
		2>>${LOGerr%.err}"_MC.err"

	# RESTART with ad hoc
	poll_till_free
	$EXE $RESTART_ADHOC $STOP_CRITERION $SPLITTINGS  \
		1>>${LOGout%.out}"_AH.out"                   \
		2>>${LOGerr%.err}"_AH.err"

	# RESTART with auto (coupled)
	poll_till_free
	$EXE $RESTART_AUTO_COUPLED $STOP_CRITERION $SPLITTINGS  \
		1>>${LOGout%.out}"_AC.out"                          \
		2>>${LOGerr%.err}"_AC.err"

	# RESTART with auto (split)
	poll_till_free
	$EXE $RESTART_AUTO_SPLIT $STOP_CRITERION $SPLITTINGS  \
		1>>${LOGout%.out}"_AS.out"                        \
		2>>${LOGerr%.err}"_AS.err"

	echo " done"
done

# Wait till termination
## echo "Waiting for all experiments to finish"
## RUNNING=`echo "pgrep -u $(whoami) fig"`
## while [ ! -z "$RUNNING" ]; do sleep 20; done
## echo "All experiments finished"

exit 0

