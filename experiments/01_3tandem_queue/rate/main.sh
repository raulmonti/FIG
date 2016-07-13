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
TO="4h"
CONF=0.9  # Confidence coefficient
PREC=0.2  # Relative precision
SPLITS=(2 3 6 11)  # RESTART splittings to test
#OCUPA=(18  13  20  16  24  21)  # estimates ~ 10^-15
OCUPA=(10   7  11   9  14  12)  # estimates ~ 5*10^-9
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
EXPNAME="3tandem_queue"
#
show "Configuring experiments"
NUM_EXPERIMENTS=${#OCUPA[@]}
STOP_CRITERION="--stop-conf $CONF $PREC"
ETIMEOUT="${TO##*[0-9]}"  # Experiment timeout (ifun&thr building + sim)
ETIMEOUT=$(bc -l <<< "scale=0; ${TO%%[a-z]*}*1.4/1")"$ETIMEOUT"
show "Timeouts: $TO per simulation; $ETIMEOUT per experiment"
STANDARD_MC="-e nosplit --flat $STOP_CRITERION --timeout $TO"
RESTART_ADHOC="--adhoc q3 $STOP_CRITERION --timeout $TO"
RESTART_AMONO="--amono $STOP_CRITERION --timeout $TO"
RESTART_ACOMP="--acomp \"+\" $STOP_CRITERION --timeout $TO"


# Launch experiments
show "Launching experiments:"
for (( i=0 ; i < NUM_EXPERIMENTS ; i++ ))
do
	L=${OCUPA[i]}
	show -n "  · for threshold occupancy = $L..."

	# Select optimal ad hoc ifun for this experiment (from V-A's paper)
	RESTART_ADHOC_OPT="--adhoc ${AHFUN[i]} $STOP_CRITERION --timeout $TO"

	# Modify model file to fit this experiment
	MODEL_FILE_L=${MODEL_FILE%.sa}"_l$L.sa"
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
	LOG=${RESULTS}/${EXPNAME}_l${L}
	EXE=`/bin/echo -e "timeout -s 15 $ETIMEOUT ./fig $MODEL_FILE_L $PROPS_FILE"`

	# Launch a job for each splitting value (improves load balance)
	for s in "${SPLITS[@]}"
	do
		# RESTART with monolithic (auto ifun)
		poll_till_free $EXPNAME; show -n " AM_s${s},"
		$EXE $RESTART_AMONO -s $s 1>>${LOG}"_AM_s${s}.out" \
		                          2>>${LOG}"_AM_s${s}.err" &

		# RESTART with compositional (auto ifun)
		poll_till_free $EXPNAME; show -n " AC_s${s},"
		$EXE $RESTART_ACOMP -s $s 1>>${LOG}"_AC_s${s}.out" \
		                          2>>${LOG}"_AC_s${s}.err" &

		# RESTART with ad hoc (default)
		poll_till_free $EXPNAME; show -n " AHD_s${s},"
		$EXE $RESTART_ADHOC -s $s 1>>${LOG}"_AHD_s${s}.out" \
		                          2>>${LOG}"_AHD_s${s}.err" &

		# RESTART with ad hoc (optimal)
		poll_till_free $EXPNAME; show -n " AHO_s${s},"
		$EXE $RESTART_ADHOC_OPT -s $s 1>>${LOG}"_AHO_s${s}.out" \
		                              2>>${LOG}"_AHO_s${s}.err" &
	done

	# Standard Monte Carlo
	poll_till_free $EXPNAME; show -n " MC"
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
for (( i=0 ; i < NUM_EXPERIMENTS ; i++ ))
do
	# Unify each importance function results in a single file
	L=${OCUPA[i]}
	LOG=${RESULTS}/${EXPNAME}_l${L}
	for IFUN in "${IFUNS[@]}"; do
		if [[ ${IFUN} == "MC" ]]; then continue; fi
		cat ${LOG}_${IFUN}_s[0-9]*.out >> ${LOG}"_${IFUN}.out"
		cat ${LOG}_${IFUN}_s[0-9]*.err >> ${LOG}"_${IFUN}.err"
		rm  ${LOG}_${IFUN}_s[0-9]*.{out,err}
	done
done
EXPERIMENTS=("${OCUPA[@]/#/l}")
build_c_table "est"  $RESULTS EXPERIMENTS[@] IFUNS[@] SPLITS[@] $CONF $PREC \
	&> $RESULTS/table_estimates.txt
build_c_table "prec" $RESULTS EXPERIMENTS[@] IFUNS[@] SPLITS[@] $CONF $PREC \
	&> $RESULTS/table_precisions.txt
build_c_table "time" $RESULTS EXPERIMENTS[@] IFUNS[@] SPLITS[@] $CONF $PREC \
	&> $RESULTS/table_times.txt
show " done"


# Turn lights off
EXE_WTIME=$(format_seconds $SECONDS)  
show "Finished on $(date)"
show "Script execution walltime was $EXE_WTIME"
show "Results are in ${RESULTS}"
exit 0

