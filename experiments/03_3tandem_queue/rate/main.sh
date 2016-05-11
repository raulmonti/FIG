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
MODEL_FILE="3tandem_queue.sa"
copy_model_file $MODEL_FILE $THIS_DIR && \
	show "  · using model file $MODEL_FILE"
PROPS_FILE="3tandem_queue.pp"
show 'S( q3 >= L )  // "rate"' > $PROPS_FILE && \
	show "  · using properties file $PROPS_FILE"
N=0; RESULTS="results_$N"
while [ -d $RESULTS ]; do N=$((N+1)); RESULTS="results_$N"; done
mkdir $RESULTS && unset N && \
	show "  · results will be stored in subdir \"${RESULTS}\""


# Experiments configuration
show "Configuring experiments"
#declare -a OCUPA=(18  13  20  16  24  21)
declare -a OCUPA=(18)
declare -a ALPHA=( 2   3   2   3   2   3)
declare -a BETA1=( 3 4.5   6   9  10  15)
declare -a BETA2=( 4   6   4   6   8  12)
declare -a BETA3=( 6   9   6   9   6   9)
declare -a AHFUN=( "0,182*q1+0,308*q2+q3" \
                   "0,132*q1+0,228*q2+q3" \
                   "0,400*q1+0,400*q2+q3" )
#                   "0,308*q1+0,308*q2+q3" \
#                   "0,810*q1+0,810*q2+q3" \
#                   "0,640*q1+0,640*q2+q3" )
NUM_EXPERIMENTS=${#OCUPA[@]}
#STOP_CRITERION="--stop-conf 0.90 0.2"  # Confidence coeff. and rel. precision
STOP_CRITERION="--stop-time 1 m"  # Confidence coeff. and rel. precision
#SPLITTINGS="--splitting 3,6,11"        # Splitting values for RESTART engine
SPLITTINGS="--splitting 4 -t fix"        # Splitting values for RESTART engine
STANDARD_MC="-e nosplit --flat $STOP_CRITERION"
RESTART_AUTO_COUPLED="--auto-coupled $STOP_CRITERION -t fix $SPLITTINGS"
RESTART_AUTO_SPLIT="--auto-split \"+\" $STOP_CRITERION -t fix $SPLITTINGS"

# Launch experiments
show "Launching experiments:"
for (( i=0 ; i < NUM_EXPERIMENTS ; i++ ))
do
	L=${OCUPA[i]}
	show -n "  · for threshold occupancy = $L..."

	# Modify model file to fit this experiment
	MODEL_FILE_L=${MODEL_FILE%.sa}"_$L.sa"
	cp $MODEL_FILE $MODEL_FILE_L
	BLANK="[[:space:]]*"
	L_DEF="^const${BLANK}int${BLANK}L${BLANK}=${BLANK}[_\-\+[:alnum:]]*;"
	A_DEF="^const${BLANK}int${BLANK}alpha${BLANK}=${BLANK}[_\-\+[:alnum:]]*;"
	B1_USE="=${BLANK}erlang(alpha,beta1));"
	B2_USE="=${BLANK}erlang(alpha,beta2));"
	B3_USE="=${BLANK}erlang(alpha,beta3));"
	sed -i -e "s/${L_DEF}/const int L = $L;/1" $MODEL_FILE_L
	sed -i -e "s/${A_DEF}/const int alpha = ${ALPHA[i]};/1" $MODEL_FILE_L
	sed -i -e "s/${B1_USE}/= erlang(alpha,${BETA1[i]}));/g" $MODEL_FILE_L
	sed -i -e "s/${B2_USE}/= erlang(alpha,${BETA2[i]}));/g" $MODEL_FILE_L
	sed -i -e "s/${B3_USE}/= erlang(alpha,${BETA3[i]}));/g" $MODEL_FILE_L
	LOG=${RESULTS}/3tandem_queue_L${L}
	EXE=`/bin/echo -e "timeout -s 15 10h ./fig $MODEL_FILE_L $PROPS_FILE"`

	# Select optimal ad hoc ifun (from V-A's paper)
	RESTART_ADHOC="--adhoc ${AHFUN[i]} $STOP_CRITERION $SPLITTINGS"

#	# Standard Monte Carlo
#	poll_till_free; show -n " MC"
#	$EXE $STANDARD_MC 1>>${LOG}"_MC.out" 2>>${LOG}"_MC.err" &

	# RESTART with ad hoc
	poll_till_free; show -n ", AH"
	$EXE $RESTART_ADHOC 1>>${LOG}"_AH.out" 2>>${LOG}"_AH.err" &

#	# RESTART with auto (coupled)
#	poll_till_free; show -n ", AC"
#	$EXE $RESTART_AUTO_COUPLED 1>>${LOG}"_AC.out" 2>>${LOG}"_AC.err" &

#	# RESTART with auto (split)
#	poll_till_free; show -n ", AS"
#	$EXE $RESTART_AUTO_SPLIT 1>>${LOG}"_AS.out" 2>>${LOG}"_AS.err" &

	show "... done"
done


# Wait till termination
show -n "Waiting for all experiments to finish..."
wait
show " done\nResults are in ${RESULTS}"


exit 0

