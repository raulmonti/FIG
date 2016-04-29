#!/bin/bash
#
# Author:  Carlos E. Budde
# Date:    22.04.2016
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
EXP_GEN="database-gen.sh"
copy_model_file $EXP_GEN $THIS_DIR && \
	$ECHO "  · using model&properties generator \"$EXP_GEN\""
N=0; RESULTS="results_$N"
while [ -d $RESULTS ]; do N=$((N+1)); RESULTS="results_$N"; done
mkdir $RESULTS && unset N && \
	$ECHO "  · results will be stored in subdir \"${RESULTS}\""


# Experiments configuration
$ECHO "Configuring experiments for 2 'RED'undancy,"
$ECHO "                            6 'D'isk clusters,"
$ECHO "                            2 'C'ontroller types,"
$ECHO "                            2 'P'rocessors types"
declare -a MEAN_FAILURE_TIMES=(2000 8000 32000 128000)
STOP_CRITERION="--stop-conf 0.95 0.2"  # Confidence coeff. and rel. precision
SPLITTINGS="--splitting 2,3,6,11"      # Splitting values for RESTART engine
MIN_OC="2-min(2-c11f-c12f,min(2-c21f-c22f,min(2-p11f-p12f,min(2-p21f-p22f,min(2-d11f-d12f-d13f-d14f,min(2-d21f-d22f-d23f-d24f,min(2-d31f-d32f-d33f-d34f,min(2-d41f-d42f-d43f-d44f,min(2-d51f-d52f-d53f-d54f,2-d61f-d62f-d63f-d64f)))))))));0;2"
STANDARD_MC="-e nosplit --flat $STOP_CRITERION"
RESTART_ADHOC="--adhoc ${MIN_OC} $STOP_CRITERION $SPLITTINGS -t ams"
RESTART_AUTO_COUPLED="--auto-coupled $STOP_CRITERION $SPLITTINGS -t ams"
RESTART_AUTO_SPLIT="--auto-split \"+\" $STOP_CRITERION $SPLITTINGS -t ams"


# Launch experiments
$ECHO "Launching experiments:"
for mft in "${MEAN_FAILURE_TIMES[@]}"
do
	$ECHO -n "  · for mean failure times ~ $mft..."

	# Generate model and properties files to fit this experiment
	MODEL_FILE=database_mft${mft}.sa
	PROPS_FILE=database_mft${mft}.pp
	LOG=${RESULTS}/database_mft${mft}
	bash $EXP_GEN 2 6 2 2 $mft 1>$MODEL_FILE 2>$PROPS_FILE
	EXE=`$ECHO "timeout -s 15 10h ./fig $MODEL_FILE $PROPS_FILE"`
  
	# Standard Monte Carlo
	poll_till_free; $ECHO -n " MC"
	$EXE $STANDARD_MC 1>>${LOG}"_MC.out" 2>>${LOG}"_MC.err" &

	# RESTART with ad hoc
	poll_till_free; $ECHO -n ", AH"
	$EXE $RESTART_ADHOC 1>>${LOG}"_AH.out" 2>>${LOG}"_AH.err" &

	# RESTART with auto-split
	poll_till_free; $ECHO -n ", AS"
	$EXE $RESTART_AUTO_SPLIT 1>>${LOG}"_AS.out" 2>>${LOG}"_AS.err" &

	# RESTART with auto-coupled experiments are omitted
	# since the importance vector wouldn't fit in memory

	$ECHO "... done"
done


# Wait till termination
$ECHO "Waiting for all experiments to finish"
declare -a A=(. :) ; N=0
RUNNING=`$ECHO "pgrep -u $(whoami) fig"`
while [ -n "`$RUNNING`" ]; do $ECHO -n ${A[$N]}; N=$(((N+1)%2)); sleep 9; done
unset A N
$ECHO
$ECHO "All experiments finished"


exit 0

