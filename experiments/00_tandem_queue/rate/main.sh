#!/bin/bash
#
# Author:  Carlos E. Budde
# Date:    22.03.2016
# License: GPLv3
#
# 10.04.2017  NOTE: Script updated to work with FIG 1.1
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
MODEL_FILE="tandem_queue.sa"
PROPERTY="S( q2 == c )"
TMP=tmp.sa
CUT_PROPS='BEGIN{flag=1}/^properties/{flag=0}/^endproperties/{next;flag=1}flag'
copy_model_file $MODEL_FILE $CWD && \
	show "  · using model file $MODEL_FILE"; \
	gawk "$CUT_PROPS" $MODEL_FILE > $TMP && \
	echo -e "\nproperties\n\t$PROPERTY\nendproperties\n" >> $TMP && \
	show "  · to study property \"$PROPERTY\""; \
	mv $TMP $MODEL_FILE
N=0; RESULTS="results_$N"
while [ -d $RESULTS ]; do N=$((N+1)); RESULTS="results_$N"; done
mkdir $RESULTS && unset N && \
	show "  · results will be stored in subdir \"${RESULTS}\""


# Experiments configuration
TO="6h"
CONF=0.9  # Confidence coefficient
PREC=0.4  # Relative precision
SPLITS=(2 5 10 15)  # RESTART splittings to test
QUEUES_CAPACITIES=(10 13 16 18 21)  # =(10 15 20 25)
EXPNAME="${MODEL_FILE%.sa}_ss"
#
show "Configuring experiments"
STOP_CRITERION="--stop-conf $CONF $PREC"
ETIMEOUT="${TO##*[0-9]}"  # Experiment timeout (ifun&thr building + sim)
ETIMEOUT=$(bc -l <<< "scale=0; ${TO%%[a-z]*}*1.4/1")"$ETIMEOUT"
show "Timeouts: $TO per simulation; $ETIMEOUT per experiment"
STANDARD_MC="--flat $STOP_CRITERION --timeout $TO"
RESTART_ADHOC="--adhoc q2 $STOP_CRITERION --timeout $TO"
RESTART_AMONO="--amono $STOP_CRITERION --timeout $TO"
RESTART_ACOMP="--acomp \"+\" $STOP_CRITERION --timeout $TO"


# Launch experiments
show "Launching experiments:"
for c in "${QUEUES_CAPACITIES[@]}"
do
	show -n "  · for queues capacity = $c..."

	# Modify model file to fit this experiment
	MODEL_FILE_C="${EXPNAME}_c${c}.sa"
	BLANK="[[:space:]]*"
	C_DEF="^const${BLANK}int${BLANK}c${BLANK}=${BLANK}[_\-\+[:alnum:]]*;"
	sed -e "s/${C_DEF}/const int c = $c;/1" $MODEL_FILE > $MODEL_FILE_C
	LOG=${RESULTS}/${EXPNAME}_c${c}
	EXE=`/bin/echo -e "timeout -s 15 $ETIMEOUT ./fig $MODEL_FILE_C"`

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
	
		# RESTART with ad hoc
		poll_till_free $EXPNAME; show -n " AH_s${s},"
		$EXE $RESTART_ADHOC -s $s 1>>${LOG}"_AH_s${s}.out" \
		                          2>>${LOG}"_AH_s${s}.err" &
	done

	# Standard Monte Carlo
	poll_till_free $EXPNAME; show -n " MC"
	$EXE $STANDARD_MC 1>>${LOG}"_MC.out" 2>>${LOG}"_MC.err" &

	show "... done"
done


# Wait till termination, making sure everything dies after the timeout
show -n "Waiting for all experiments to finish..."
`PIDS=$(ps -fC "fig" | grep $EXPNAME | awk '{ print $2 }') \
 sleep $ETIMEOUT; kill -15 $PIDS &>/dev/null;              \
 sleep 2;         kill  -9 $PIDS &>/dev/null`              &
disown %%; wait &>/dev/null;   # killall sleep &>/dev/null
show " done"


# Build summary charts
IFUNS=("MC" "AH" "AC" "AM")  # <-- reflect any change in the plotting section
RAW_RESULTS=${RESULTS}/raw_results; mkdir -p $RAW_RESULTS
MRG_RESULTS=${RESULTS}/mrg_results; mkdir -p $MRG_RESULTS
show -n "Merging results..."
for c in "${QUEUES_CAPACITIES[@]}"; do
	# Unify each importance function results in a single file
	LOG=${RESULTS}/${EXPNAME}_c${c}
	cp ${LOG}_MC.{out,err} ${RAW_RESULTS}  # MC is special, as usual
	for IFUN in "${IFUNS[@]}"; do
		if [[ ${IFUN} == "MC" ]]; then continue; fi
		cat ${LOG}_${IFUN}_s[0-9]*.out >> ${LOG}"_${IFUN}.out"
		cat ${LOG}_${IFUN}_s[0-9]*.err >> ${LOG}"_${IFUN}.err"
		mv  ${LOG}_${IFUN}_s[0-9]*.{out,err} ${RAW_RESULTS}
	done
done
show " done"
#
show -n "Building tables..."
EXPERIMENTS=("${QUEUES_CAPACITIES[@]/#/c}")
build_c_table "est"  $RESULTS EXPERIMENTS[@] IFUNS[@] SPLITS[@] $CONF $PREC \
	> $RESULTS/table_estimates.txt
build_c_table "prec" $RESULTS EXPERIMENTS[@] IFUNS[@] SPLITS[@] $CONF $PREC \
	> $RESULTS/table_precisions.txt
build_c_table "time" $RESULTS EXPERIMENTS[@] IFUNS[@] SPLITS[@] $CONF $PREC \
	> $RESULTS/table_times.txt
mv ${RESULTS}/*.{out,err} ${MRG_RESULTS}
show " done"


# Turn lights off
EXE_WTIME=$(format_seconds $SECONDS)  
show "Finished on $(date)"
show "Script execution walltime was $EXE_WTIME"
show "Results are in ${RESULTS}"
exit 0

