#!/bin/bash
#
# Author:  Carlos E. Budde
# Date:    03.04.2017
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
source "$CWD/../ifuns.sh" || \
	(show "[ERROR] Couldn't find ifuns.sh" && exit 1)
if [[ "$(type -t max_continuous_failures_adhoc)" != "function" ]]
then
	show "[ERROR] Bash function \"max_continuous_failures_adhoc\" is undefined"
	exit 1;
elif [[ "$(type -t max_continuous_failures_acomp)" != "function" ]]
then
	show "[ERROR] Bash function \"max_continuous_failures_acomp\" is undefined"
	exit 1;
elif [[ "$(type -t sum_continuous_failures_acomp)" != "function" ]]
then
	show "[ERROR] Bash function \"sum_continuous_failures_acomp\" is undefined"
	exit 1;
fi


# Build project
show "Building FIG"
build_fig $CWD
if [ ! -f ./fig ]; then show "[ERROR] Something went wrong"; exit 1; fi


# Prepare experiment's directory and files
show "Preparing experiments environment:"
EXP_GEN="oil_pipeline-gen.sh"
copy_model_file $EXP_GEN $CWD "link" && \
	show "  · using model&properties generator \"$EXP_GEN\""
N=0; RESULTS="results_$N"
while [ -d $RESULTS ]; do N=$((N+1)); RESULTS="results_$N"; done
mkdir $RESULTS && unset N && \
	show "  · results will be stored in subdir \"${RESULTS}\""


# Experiments configuration
FAIL_DISTRIBUTIONS=("exponential(0.001)" "rayleigh(729)")
PARAM_N=(20)  # (20 40 60)
PARAM_K=(3 4 5)
TIME_BOUNDS=(90m 180m 6h)  # one per vale in $PARAM_K
CONF=0.9  # Confidence coefficient
PREC=0.4  # Relative precision
SPLITS=(2 5 10 15)  # RESTART splittings to test
EXPNAME="oilpipe"
#
show "Configuring experiments"
STOP_CRITERION="--stop-conf $CONF $PREC"


# Launch experiments
for FDIST in "${FAIL_DISTRIBUTIONS[@]}"; do  # Distributions loop
#
	show "Launching experiments (${FDIST%(*} failures):"
#
for (( i=0 ; i<${#PARAM_K[*]} ; i++ )); do   # K loop
#
	K=${PARAM_K[i]}
#
for N in "${PARAM_N[@]}"; do                 # N loop
#
	show -n "  · for $K-out-of-$N ..."

	# Experiment's time limits
	TB=${TIME_BOUNDS[i]}
	if [ `compute_seconds $TB` -lt 600 ]  # Experiment TO: ifun + thr + sim
	then
		ETIMEOUT="10m"
	else
		ETIMEOUT="${TB##*[0-9]}"
		ETIMEOUT=$(bc -l <<< "scale=0; ${TB%%[a-z]*}*2/1")"$ETIMEOUT"
	fi

	# Experiment's importance functions
	MAX_CF_AH=$(max_continuous_failures_adhoc $N $K)
	MAX_CF_AC=$(max_continuous_failures_acomp $N $K)
	SUM_CF_AC=$(sum_continuous_failures_acomp $N $K)

	# Experiment's execution commands
	RESTART_ACOMP1="--acomp \"+\" $STOP_CRITERION --timeout $TB"
	RESTART_ACOMP2="--acomp \"*\" $STOP_CRITERION --timeout $TB --post-process exp 2"
	RESTART_ACOMP3="--acomp $MAX_CF_AC $STOP_CRITERION --timeout $TB"
	RESTART_ACOMP4="--acomp $SUM_CF_AC $STOP_CRITERION --timeout $TB --post-process exp 2"
	RESTART_ADHOC=" --adhoc $MAX_CF_AH $STOP_CRITERION --timeout $TB"
	STANDARD_MC="--flat $STOP_CRITERION --timeout $TB"

	# Experiment's model and properties
	MODEL_FILE=${EXPNAME}_${FDIST:0:3}_N${N}_K${K}.sa
	LOG=${RESULTS}/${EXPNAME}_${FDIST:0:3}_N${N}_K${K}
	bash $EXP_GEN $N $K "$FDIST" "lognormal(1.21,0.8)" $MODEL_FILE
	EXE=`/bin/echo -e "timeout -s 15 $ETIMEOUT ./fig $MODEL_FILE"`

	# Launch an independent job for each splitting value (improves load balance)
	for s in "${SPLITS[@]}";
	do
		# RESTART with monolithic (auto ifun) experiments are omitted
		# since the importance vector wouldn't fit in memory

		# RESTART with compositional (auto ifun), version 1
		poll_till_free $EXPNAME; show -n " AC1_s${s},"
		$EXE $RESTART_ACOMP1 -s $s 1>>${LOG}"_AC1_s${s}.out" \
		                           2>>${LOG}"_AC1_s${s}.err" &
	
		# RESTART with compositional (auto ifun), version 2
		poll_till_free $EXPNAME; show -n " AC2_s${s},"
		$EXE $RESTART_ACOMP2 -s $s 1>>${LOG}"_AC2_s${s}.out" \
		                           2>>${LOG}"_AC2_s${s}.err" &
	
		# RESTART with compositional (auto ifun), version 3
		poll_till_free $EXPNAME; show -n " AC3_s${s},"
		$EXE $RESTART_ACOMP3 -s $s 1>>${LOG}"_AC3_s${s}.out" \
		                           2>>${LOG}"_AC3_s${s}.err" &
	
		# RESTART with compositional (auto ifun), version 4
		poll_till_free $EXPNAME; show -n " AC4_s${s},"
		$EXE $RESTART_ACOMP4 -s $s 1>>${LOG}"_AC4_s${s}.out" \
		                           2>>${LOG}"_AC4_s${s}.err" &
	
		# RESTART with ad hoc
		poll_till_free $EXPNAME; show -n " AH_s${s},"
		$EXE $RESTART_ADHOC -s $s 1>>${LOG}"_AH_s${s}.out" \
		                          2>>${LOG}"_AH_s${s}.err" &
	done

	# Standard Monte Carlo
	poll_till_free $EXPNAME; show -n " MC"
	$EXE $STANDARD_MC 1>>${LOG}"_MC.out" 2>>${LOG}"_MC.err" &

	show "... done"

done  # N loop
done  # K loop
done  # Distributions loop


# Wait till termination, making sure everything dies after the timeout
show -n "Waiting for all experiments to finish..."
TO=$(get_max_time TIME_BOUNDS[@])
ETIMEOUT="${TO##*[0-9]}"  # Experiment timeout (ifun&thr building + sim)
ETIMEOUT=$(bc -l <<< "scale=0; ${TO%%[a-z]*}*1.4/1")"$ETIMEOUT"
`PIDS=$(ps -fC "fig" | grep $EXPNAME | awk '{ print $2 }') \
 sleep $ETIMEOUT; kill -15 $PIDS &>/dev/null;              \
 sleep 2;         kill  -9 $PIDS &>/dev/null`              &
disown %%; wait &>/dev/null;  # killall sleep &>/dev/null
show " done"


# TODO: Build summary charts!
show "TODO: build charts"
## show -n "Building tables..."
## IFUNS=("MC" "AH" "AC")
## EXPERIMENTS=("${???[@]}")
## build_table "est"  $RESULTS EXPERIMENTS[@] IFUNS[@] SPLITS[@] $CONF $PREC \
## 	&> $RESULTS/table_estimates.txt
## build_table "time" $RESULTS EXPERIMENTS[@] IFUNS[@] SPLITS[@] $CONF $PREC \
## 	&> $RESULTS/table_times.txt
## show " done"


# Turn lights off
EXE_WTIME=$(format_seconds $SECONDS)  
show "Finished on $(date)"
show "Script execution walltime was $EXE_WTIME"
show "Results are in ${RESULTS}"
exit 0

