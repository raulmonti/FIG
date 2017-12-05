#!/usr/bin/env bash
#
# Author:  Carlos E. Budde
# Date:    04.12.2017
# License: GPLv3
#

set -e
show(){ /bin/echo -e "$@"; }
CWD=`readlink -f "$(dirname ${BASH_SOURCE[0]})"`


# Probe resources & Build project
source "$CWD/../../fig_utils.sh" || \
	(show "[ERROR] Couldn't find fig_utils.sh" && exit 1)
if [[ "$(type -t build_fig)" != "function" ]]; then
	show "[ERROR] Bash function \"build_fig\" is undefined";
	exit 1;
elif [[ "$(type -t copy_model_file)" != "function" ]]; then
	show "[ERROR] Bash function \"copy_model_file\" is undefined";
	exit 1;
fi
show "Building FIG"
build_fig $CWD
if [ ! -f ./fig ]; then show "[ERROR] Failed building FIG"; exit 1; fi


# Prepare experiment's directory and files
show "Preparing experiments environment:"
source "$CWD/../fetch_models.sh" || \
	(show "[ERROR] Couldn't find fetch_models.sh" && exit 1)
if [ -z $MODEL_LIST ] || [ ${#MODEL_LIST[@]} -lt 1 ] ; then
	show "[ERROR] Couldn't find the list of models to work with";
	exit 1;
fi
N=0; RESULTS="results_$N"
while [ -d $RESULTS ]; do N=$((N+1)); RESULTS="results_$N"; done
mkdir $RESULTS && unset N && \
	show "  · results will be stored in subdir \"${RESULTS}\""


# Experiments configuration
EXPNAME="hecs"
CONF=0.9  # Confidence coefficient
PREC=0.4  # Relative precision
TIME_BOUNDS=(\
	"30m"
	"120m"
)  # simulation time bounds
GEFFORT=(\
	"2"
	"5"
)  # global effort values
ISCONFIG=(\
	"-e restart -t es"
	"-e restart -t hyb"
	"-e fixedeffort -t es"
	"-e fixedeffort -t hyb"
)  # importance splitting configurations
STOP_CRITERION="--stop-conf $CONF $PREC"
show "  · conf. lvl.:  `bc <<< "scale=0;$CONF*100/1"`%"
show "  · rel. prec.:  `bc <<< "scale=0;$PREC*100/1"`%"
show "  · time limits: (${TIME_BOUNDS[*]})"
show "  · global eff:  (${GEFFORT[*]})"
#show "  · IS configs:  (${ISCONFIG[*]})"
SYS_TO_WRAP="time timeout -s 15 -k 5 "


# Launch experiments
for TB  in "${TIME_BOUNDS[@]}"; do
	RUN_SMC=true;
for GE  in "${GEFFORT[@]}"; do
	# Experiment's time limits (ifun + thr + sim)
	if [ `compute_seconds $TB` -lt 600 ]; then
		ETIMEOUT="10m";
	else
		ETIMEOUT="${TB##*[0-9]}";
		ETIMEOUT=$(bc -l <<< "scale=0; ${TB%%[a-z]*}*2/1")"$ETIMEOUT";
	fi
for CFG in "${ISCONFIG[@]}"; do
	show "Configuration \"-g $GE --timeout $TB $CFG\"";
for MODEL in "${MODEL_LIST[@]}"; do
	show "  · Running experiments on model $MODEL";
	copy_model_file $MODEL $CWD/../models/ "link";
	MODEL_FILE=$CWD/../models/$MODEL;
	# Logs
	CFG_STR=${CFG//\ /_};
	CFG_STR=${CFG_STR//\-?_/};
	CFG_STR="${CFG_STR}_${GE}_${TB}";
	LOG="${RESULTS}/${MODEL%%_with*}_${CFG_STR}";
	# Compositional ifun  (in last line of model file)
	IFUNC="`tail -n 1 $MODEL_FILE | sed 's/\/\/\ *//'`";
	if [ `echo $IFUNC | wc -c` -lt 8 ]; then
		show "[ERROR] No compositional ifun. found in model \"$MODEL\"";
		continue;
	fi
	EXE_FIG="$SYS_TO_WRAP $ETIMEOUT"
	EXE_FIG+=" ./fig $MODEL_FILE $STOP_CRITERION -g $GE --timeout $TB";
	EXE_FIG+=" $CFG --acomp '$IFUNC'"
	# Execution
	poll_till_free $EXPNAME;
	eval $EXE_FIG 1>>${LOG}".out" 2>>${LOG}".err" &

	# For each TimeBound run Standard Monte Carlo once
	if $RUN_SMC ; then
		LOG="${RESULTS}/${MODEL%%_with*}_flat_${TB}";
		EXE_FIG="$SYS_TO_WRAP $ETIMEOUT"
		EXE_FIG+=" ./fig $MODEL_FILE $STOP_CRITERION --timeout $TB";
		EXE_FIG+=" --flat";
		poll_till_free $EXPNAME;
		eval $EXE_FIG 1>>${LOG}".out" 2>>${LOG}".err" &
		RUN_SMC=false;
	fi

done  # loop MODEL in MODEL_LIST
done  # loop CFG   in ISCONFIG
done  # loop GE    in GEFFORT
done  # loop TB    in TIME_BOUNDS


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

