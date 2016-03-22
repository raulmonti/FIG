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
BUILD_DIR=`realpath ./bin`
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
EXE=`echo $FIG $MODEL_FILE $PROPS_FILE`
STANDARD_MC="nosplit algebraic flat ams"
RESTART_ADHOC="restart algebraic adhoc ams"
RESTART_AUTO_COUPLED="restart concrete_coupled auto ams"
RESTART_AUTO_SPLIT="restart concrete_split auto ams"
SPLITTINGS="2 3 5 9"       # Splitting values to test with the RESTART engine
STOP_CRITERION="0.95 0.2"  # Confidence coefficient and relative precision
declare -a QUEUES_CAPACITIES=(8 10 12 14)

# Launch experiments
echo "Launching experiments:"
for c in "${QUEUES_CAPACITIES[@]}"
do
	echo -n "\t· for queues capacity = $c..."
	LOGout=${RESULTS}/tandem_queue_c${c}.out
	LOGerr=${RESULTS}/tandem_queue_c${c}.err
	# Standard Monte Carlo
	$EXE $STANDARD_MC $STOP_CRITERION "0" 1>>$LOGout 2>>$LOGerr
	poll_till_free

	# TODO launch all the other experiments

	echo " done"
done

echo "Finished tests"

exit 0

