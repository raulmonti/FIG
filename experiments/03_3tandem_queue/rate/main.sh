#!/bin/bash
#
# Author:  Carlos E. Budde
# Date:    22.03.2016
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
MODEL_FILE="3tandem_queue.sa"
copy_model_file $MODEL_FILE $CWD && \
	show "  · using model file $MODEL_FILE"
PROPS_FILE="3tandem_queue.pp"
echo 'S( q3 >= L )  // "rate"' > $PROPS_FILE && \
	show "  · using properties file $PROPS_FILE"
N=0; RESULTS="results_$N"
while [ -d $RESULTS ]; do N=$((N+1)); RESULTS="results_$N"; done
mkdir $RESULTS && unset N && \
	show "  · results will be stored in subdir \"${RESULTS}\""


# Experiments configuration
TO="10h"
CONF=0.9  # Confidence coefficient
PREC=0.2  # Relative precision
SPLITS=(3 6 11)  # RESTART splittings to test
#OCUPA=(18  13  20  16  24  21)  # estimates ~ 10^-15
OCUPA=(11   8  12   9  14  13)  # estimates ~ 10^-9
ALPHA=( 2   3   2   3   2   3)
BETA1=( 3 4.5   6   9  10  15)
BETA2=( 4   6   4   6   8  12)
BETA3=( 6   9   6   9   6   9)
AHFUN=( "91*q1+154*q2+500*q3" \
        "66*q1+114*q2+500*q3" \
       "200*q1+200*q2+500*q3" \
       "154*q1+154*q2+500*q3" \
       "405*q1+405*q2+500*q3" \
       "320*q1+320*q2+500*q3" )
EXPNAME="3tandem"
#
show "Configuring experiments"
NUM_EXPERIMENTS=${#OCUPA[@]}
SPLITTING="--splitting "
for S in "${SPLITS[@]}"; do SPLITTING+="$S,"; done; SPLITTING="${SPLITTING%,}"
STOP_CRITERION="--stop-conf $CONF $PREC"
ETIMEOUT="${TO##*[0-9]}"  # Timeout per experiment (one ifun, all splits)
ETIMEOUT=$(bc <<< "${TO%%[a-z]*}*${#SPLITS[@]}*2")"$ETIMEOUT"
show "Timeouts: $TO per split; $ETIMEOUT per experiment"
STANDARD_MC="-e nosplit --flat $STOP_CRITERION --timeout $TO"
RESTART_ADHOC="--adhoc q3 $STOP_CRITERION $SPLITTING --timeout $TO"
RESTART_AMONO="--amono $STOP_CRITERION $SPLITTING --timeout $TO"
RESTART_ACOMP="--acomp \"+\" $STOP_CRITERION $SPLITTING --timeout $TO"


# Launch experiments
show "Launching experiments:"
for (( i=0 ; i < NUM_EXPERIMENTS ; i++ ))
do
	L=${OCUPA[i]}
	show -n "  · for threshold occupancy = $L..."

	# Select optimal ad hoc ifun for this experiment (from V-A's paper)
	RESTART_ADHOC_OPT="--adhoc ${AHFUN[i]} $STOP_CRITERION $SPLITTING --timeout $TO"

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
	EXE=`/bin/echo -e "timeout -s 15 $ETIMEOUT ./fig $MODEL_FILE_L $PROPS_FILE"`

	# RESTART with monolithic (auto ifun)
	poll_till_free $EXPNAME; show -n " AM"
	$EXE $RESTART_AMONO 1>>${LOG}"_AM.out" 2>>${LOG}"_AM.err" &

	# RESTART with compositional (auto ifun)
	poll_till_free $EXPNAME; show -n ", AC"
	$EXE $RESTART_ACOMP 1>>${LOG}"_AC.out" 2>>${LOG}"_AC.err" &

	# RESTART with ad hoc (optimal)
	poll_till_free $EXPNAME; show -n ", AHO"
	$EXE $RESTART_ADHOC_OPT 1>>${LOG}"_AHO.out" 2>>${LOG}"_AHO.err" &

	# RESTART with ad hoc (default)
	poll_till_free $EXPNAME; show -n ", AHD"
	$EXE $RESTART_ADHOC 1>>${LOG}"_AHD.out" 2>>${LOG}"_AHD.err" &

	# Standard Monte Carlo
	poll_till_free $EXPNAME; show -n ", MC"
	$EXE $STANDARD_MC 1>>${LOG}"_MC.out" 2>>${LOG}"_MC.err" &

	show "... done"
done


# Wait till termination, making sure everything dies after $TO
show -n "Waiting for all experiments to finish..."
`PIDS=$(ps -fC "fig" | grep $EXPNAME | awk '{ print $2 }') \
 sleep $ETIMEOUT; kill -15 $PIDS &>/dev/null;              \
 sleep 2;         kill  -9 $PIDS &>/dev/null`              &
disown %%; wait &>/dev/null; killall sleep &>/dev/null
show " done"


# Build summary charts
show -n "Building tables..."
IFUNS=("MC" "AHD" "AHO" "AC" "AM")
EXPERIMENTS=("${OCUPA[@]}")
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

