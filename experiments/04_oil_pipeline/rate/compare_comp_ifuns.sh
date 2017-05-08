#!/bin/bash
#
# Author:  Carlos E. Budde
# Date:    04.05.2017
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
N=0; RESULTS="ifuncomp_$N"
while [ -d $RESULTS ]; do N=$((N+1)); RESULTS="ifuncomp_$N"; done
mkdir $RESULTS && unset N && \
	show "  · results will be stored in subdir \"${RESULTS}\""


# Experiments configuration
FAIL_DISTRIBUTIONS=("exponential(0.001)" "rayleigh(729)")
PARAM_N=(20 60)  # (20 40 60)
PARAM_K=(3 4 5)
TIME_BOUNDS=(60m 120m 240m)  # one per vale in $PARAM_K
CONF=0.8  # Confidence coefficient
PREC=0.6  # Relative precision
SPLITS=(3 8)  # RESTART splittings to test
EXPNAME="oilpipe_ifun_comp"
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
	AC_DEF=$(sum_continuous_failures_acomp $N $K)
	if   [ $K -eq 3 ]; then
		AC_OPT=$(sum_continuous_failures_acomp_K3 $N)
	elif [ $K -eq 4 ]; then
		AC_OPT=$(sum_continuous_failures_acomp_K4 $N)
	elif [ $K -eq 5 ]; then
		AC_OPT=$(sum_continuous_failures_acomp_K5 $N)
	else
		show "This script is not prepared for K>5"
		exit 1
	fi

	# Experiment's execution commands
	RESTART_DEF_ACOMP="--acomp $AC_DEF $STOP_CRITERION --timeout $TB --post-process exp 2"
	RESTART_OPT_ACOMP="--acomp $AC_OPT $STOP_CRITERION --timeout $TB --post-process exp 2"

	# Experiment's model and properties
	MODEL_FILE=${EXPNAME}_${FDIST:0:3}_N${N}_K${K}.sa
	LOG=${RESULTS}/${EXPNAME}_${FDIST:0:3}_N${N}_K${K}
	bash $EXP_GEN $N $K "$FDIST" "lognormal(1.21,0.8)" $MODEL_FILE
	EXE=`/bin/echo -e "timeout -s 15 $ETIMEOUT ./fig $MODEL_FILE"`

	# Launch an independent job for each splitting value (improves load balance)
	for s in "${SPLITS[@]}";
	do
		# RESTART with default compositional ifun
		poll_till_free $EXPNAME; show -n " ACD_s${s},"
		$EXE $RESTART_DEF_ACOMP -s $s 1>>${LOG}"_ACD_s${s}.out" \
		                              2>>${LOG}"_ACD_s${s}.err" &

		# RESTART with optimised compositional ifun
		poll_till_free $EXPNAME; show -n " ACO_s${s},"
		$EXE $RESTART_OPT_ACOMP -s $s 1>>${LOG}"_ACO_s${s}.out" \
		                              2>>${LOG}"_ACO_s${s}.err" &
	done

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

