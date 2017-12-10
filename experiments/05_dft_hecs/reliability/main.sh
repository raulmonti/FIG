#!/usr/bin/env bash
#
# Author:  Carlos E. Budde
# Date:    04.12.2017
# License: GPLv3
#

set -e;
show(){ /bin/echo -e "$@"; }       # echo with regexp
showe(){ /bin/echo -e "$@" >&2; }  # echo with regexp to stderr
CWD=`readlink -f "$(dirname ${BASH_SOURCE[0]})"`;


# Probe resources & Build project
source "$CWD/../../fig_utils.sh" || \
	(showe "[ERROR] Couldn't find fig_utils.sh" && exit 1)
if [[ "$(type -t build_fig)" != "function" ]]; then
	showe "[ERROR] Bash function \"build_fig\" is undefined";
	exit 1;
elif [[ "$(type -t copy_model_file)" != "function" ]]; then
	showe "[ERROR] Bash function \"copy_model_file\" is undefined";
	exit 1;
fi
show "Building FIG";
build_fig $CWD;
if [ ! -f ./fig ]; then
	showe "[ERROR] Failed building FIG";
	exit 1;
fi
MAXJOBSN=`gawk '/cpu[[:space:]]*cores/{print $4; exit;}' /proc/cpuinfo`;
MAXJOBSN=$((MAXJOBSN*2));
if [ -z $MAXJOBSN ] || [ $MAXJOBSN -le 0 ]; then
	MAXJOBSN=`nproc --all`;
fi
export MAXJOBSN=$MAXJOBSN;
show "Using $MAXJOBSN parallel processors in host \"`hostname`\"";


# Prepare experiment's directory and files
show "Preparing experiments environment:"
source "$CWD/fetch_models.sh" || \
	(showe "[ERROR] Couldn't find fetch_models.sh" && exit 1)
if [ -z $MODEL_LIST ] || [ ${#MODEL_LIST[@]} -lt 1 ] ; then
	showe "[ERROR] Couldn't find the list of models to work with";
	exit 1;
fi
N=0; RESULTS="results_$N";
while [ -d $RESULTS ]; do N=$((N+1)); RESULTS="results_$N"; done
mkdir -p models;
mkdir $RESULTS && unset N && \
	show "  · results will be stored in subdir \"${RESULTS}\"";


# Experiments configuration
EXPNAME="hecs/reliability0";
CONF=0.9;  # Confidence coefficient
PREC=0.4;  # Relative precision
TIME_BOUNDS=(\
	"60m"
	"180m"
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
STOP_CRITERION="--stop-conf $CONF $PREC";
UNIX_TIMEOUT="time timeout -s 15 -k 5 ";
show "  · conf. lvl.:  `bc <<< "scale=0;$CONF*100/1"`%";
show "  · rel. prec.:  `bc <<< "scale=0;$PREC*100/1"`%";
show "  · time limits: (${TIME_BOUNDS[*]})";
show "  · global eff:  (${GEFFORT[*]})";


# Run experiments
show "Enqueueing all jobs in SLURM...";
ISPLIT_CFG=("flat");
LOG_ISCONFIG_STR=true;
for TB  in "${TIME_BOUNDS[@]}"; do
	RUN_SMC=true;  # Run Standard Monte Carlo, once for each time bound
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
	CFG_STR=${CFG//\ /_};
	CFG_STR=${CFG_STR//\-?_/};
	if $LOG_ISCONFIG_STR ; then
		ISPLIT_CFG+=("$CFG_STR");
	fi
	CFG_STR="${CFG_STR}_${TB}_${GE}";
for MODEL in "${MODEL_LIST[@]}"; do
	show "  · Running experiments on model $MODEL";
	copy_model_file $MODEL $CWD/models;
	MODEL=$(basename $MODEL);
	MODEL_FILE=$CWD/models/$MODEL;
	# Logs
	LOG="${RESULTS}/${MODEL%%_with*}_${CFG_STR}";
	# Compositional ifun  (in last line of model file)
	IFUNC="`tail -n 1 $MODEL_FILE | sed 's/\/\/\ *//'`";
	IFUNC=${IFUNC//\ /};
	if [ `echo $IFUNC | wc -c` -lt 8 ]; then
		showe "[ERROR] No compositional ifun. found in model \"$MODEL\"";
		continue;
	fi
	EXE_FIG="$UNIX_TIMEOUT $ETIMEOUT";
	EXE_FIG+=" ./fig $MODEL_FILE $STOP_CRITERION -g $GE --timeout $TB";
	EXE_FIG+=" $CFG --dft --acomp '$IFUNC'"
	# Execution
	poll_till_free $EXPNAME;
	eval $EXE_FIG 1>>${LOG}".out" 2>>${LOG}".err" &

	# If requested, for each Model run also Standard Monte Carlo
	if $RUN_SMC ; then
		LOG="${RESULTS}/${MODEL%%_with*}_flat_${TB}";
		EXE_FIG="$UNIX_TIMEOUT $ETIMEOUT"
		EXE_FIG+=" ./fig $MODEL_FILE $STOP_CRITERION --timeout $TB";
		EXE_FIG+=" --flat";
		poll_till_free $EXPNAME;
		eval $EXE_FIG 1>>${LOG}".out" 2>>${LOG}".err" &
	fi

done  # loop MODEL in MODEL_LIST
	RUN_SMC=false;  # Standard Monte Carlo runs once per time bound
done  # loop CFG   in ISCONFIG
	LOG_ISCONFIG_STR=false;
done  # loop GE    in GEFFORT
done  # loop TB    in TIME_BOUNDS
show " done";


# Wait till termination, making sure everything dies after the timeout
show -n "Waiting for all experiments to finish...";
TO=$(get_max_time TIME_BOUNDS[@])
ETIMEOUT="${TO##*[0-9]}"  # Experiment timeout (ifun&thr building + sim)
ETIMEOUT=$(bc -l <<< "scale=0; ${TO%%[a-z]*}*1.4/1")"$ETIMEOUT"
`PIDS=$(ps -fC "fig" | grep $EXPNAME | awk '{ print $2 }') \
 sleep $ETIMEOUT; kill -15 $PIDS &>/dev/null;              \
 sleep 2;         kill  -9 $PIDS &>/dev/null`              &
disown %%; wait &>/dev/null;  # killall sleep &>/dev/null
show " done";


# Build summary charts
set +e;
CHARTS_SCRIPT=build_chart.sh;
if [ ! -f $CHARTS_SCRIPT ]; then
	showe "[ERROR] Can't build summary tables: file $CHARTS_SCRIPT not found";
	exit 1;
fi
show -n "Building summary tables...";
TITLE="hecs${EXPNAME: -1}";  # https://stackoverflow.com/a/17542946
for TB  in "${TIME_BOUNDS[@]}"; do
for GE  in "${GEFFORT[@]}"; do
	# "source" to run in same environment
	source $CHARTS_SCRIPT "$TB" "$GE" "$TITLE" \
		> ${RESULTS}/chart_${TITLE}_${TB}_${GE}.txt;
done
done
RAW_RESULTS=${RESULTS}/raw_results;
mkdir -p $RAW_RESULTS;
mv ${RESULTS}/*.{out,err} $RAW_RESULTS;
show " done";


# Turn lights off
EXE_WTIME=$(format_seconds $SECONDS);
show "Finished on $(date)";
show "Script execution walltime was $EXE_WTIME";
show "Results are in ${RESULTS}";
exit 0;

