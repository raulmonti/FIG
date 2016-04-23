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
MODEL_FILE="database.sa"
copy_model_file $MODEL_FILE $THIS_DIR && \
	$ECHO "  · using model file $MODEL_FILE"
PROPS_FILE="database.pp"
copy_model_file $PROPS_FILE $THIS_DIR && \
	$ECHO "  · using properties file $PROPS_FILE"
N=0; RESULTS="results_$N"
while [ -d $RESULTS ]; do N=$(($N+1)); RESULTS="results_$N"; done
mkdir $RESULTS && unset N && \
	$ECHO "  · results will be stored in subdir \"${RESULTS}\""


# Experiments configuration
$ECHO "Configuring experiments"
STOP_CRITERION="--stop-conf 0.95 0.2"  # Confidence coeff. and rel. precision
SPLITTINGS="--splitting 2,3,6"         # Splitting values for RESTART engine
MIN_OC="2-min(2-c11f-c12f, min(2-c21f-c22f, min(2-p11f-p12f, min(2-p21f-p22f,
	min(2-d11f-d12f-d13f-d14f, min(2-d21f-d22f-d23f-d24f,
	min(2-d31f-d32f-d33f-d34f, min(2-d41f-d42f-d43f-d44f,
	min(2-d51f-d52f-d53f-d54f, 2-d61f-d62f-d63f-d64f)))))))));0;2"
STANDARD_MC="-e nosplit --flat $STOP_CRITERION"
RESTART_ADHOC="--adhoc \"${MIN_OC}\" $STOP_CRITERION $SPLITTINGS -t \"ams\""
RESTART_AUTO_COUPLED="--auto-coupled $STOP_CRITERION $SPLITTINGS -t \"ams\""
RESTART_AUTO_SPLIT="--auto-split \"+\" $STOP_CRITERION $SPLITTINGS -t \"ams\""


# Launch experiments
  LOGout=${RESULTS}/database.out
  LOGerr=${RESULTS}/database.err
  EXE=`$ECHO "timeout -s 15 10h ./fig $MODEL_FILE $PROPS_FILE"`
  
  poll_till_free
  # Standard Monte Carlo
  $ECHO -n " MC"
  $EXE $STANDARD_MC 1>>${LOGout%.out}"_MC.out" \
                    2>>${LOGerr%.err}"_MC.err" &
  poll_till_free
  # RESTART with ad hoc
  $ECHO -n ", AH"
  $EXE $RESTART_ADHOC 1>>${LOGout%.out}"_AH.out" \
                      2>>${LOGerr%.err}"_AH.err" &
  poll_till_free
  # RESTART with auto (coupled)
  $ECHO -n ", AC"
  $EXE $RESTART_AUTO_COUPLED 1>>${LOGout%.out}"_AC.out" \
                             2>>${LOGerr%.err}"_AC.err" &
  poll_till_free
  # RESTART with auto (split)
  $ECHO -n ", AS"
  $EXE $RESTART_AUTO_SPLIT 1>>${LOGout%.out}"_AS.out"  \
                           2>>${LOGerr%.err}"_AS.err"  &
$ECHO "... done"


# Wait till termination
$ECHO "Waiting for all experiments to finish"
declare -a A=(. :) ; N=0
RUNNING=`$ECHO "pgrep -u $(whoami) fig"`
while [ -n "`$RUNNING`" ]; do $ECHO -n ${A[$N]}; N=$((($N+1)%2)); sleep 9; done
unset A N
$ECHO
$ECHO "All experiments finished"


exit 0

