#!/bin/bash
#
# Author:  Carlos E. Budde
# Date:    22.03.2016
# License: GPLv3
#

#set -e
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
MODEL_FILE="queues_with_breakdowns.sa"
copy_model_file $MODEL_FILE $CWD && \
	show "  · using model file $MODEL_FILE"
PROPS_FILE="queues_with_breakdowns.pp"
echo 'P( !reset U buf == K )' > $PROPS_FILE && \
	show "  · using properties file $PROPS_FILE"
N=0; RESULTS="results_$N"
while [ -d $RESULTS ]; do N=$((N+1)); RESULTS="results_$N"; done
mkdir $RESULTS && unset N && \
	show "  · results will be stored in subdir \"${RESULTS}\""


# Experiments configuration
TO="6h"
CONF=0.9  # Confidence coefficient
PREC=0.4  # Relative precision
SPLITS=(3 6 9 12)  # RESTART splittings to test
#BUFFER_CAPACITIES=(40 80 120 160)
BUFFER_CAPACITIES=(40 60 80 100 120 140 160)
EXPNAME="queues_with_breakdowns"
#
show "Configuring experiments"
STOP_CRITERION="--stop-conf $CONF $PREC"
ETIMEOUT="${TO##*[0-9]}"  # Experiment timeout (ifun&thr building + sim)
ETIMEOUT=$(bc -l <<< "scale=0; ${TO%%[a-z]*}*1.4/1")"$ETIMEOUT"
show "Timeouts: $TO per simulation; $ETIMEOUT per experiment"
STANDARD_MC="-e nosplit --flat $STOP_CRITERION --timeout $TO"
RESTART_ADHOC="--adhoc buf $STOP_CRITERION --timeout $TO"
RESTART_AMONO="--amono $STOP_CRITERION --timeout $TO"
RESTART_ACOMP="--acomp \"+\" $STOP_CRITERION --timeout $TO"


# Launch experiments
show "Launching experiments:"
for k in "${BUFFER_CAPACITIES[@]}"
do
	show -n "  · for buffer capacity = $k..."

	# Modify model file to fit this experiment
	MODEL_FILE_K=${MODEL_FILE%.sa}"_k${k}.sa"
	BLANK="[[:space:]]*"
	K_DEF="^const${BLANK}int${BLANK}K${BLANK}=${BLANK}[_\-\+[:alnum:]]*;"
	sed -e "s/${K_DEF}/const int K = $k;/1" $MODEL_FILE > $MODEL_FILE_K
	LOG=${RESULTS}/${EXPNAME}_k${k}
	EXE=`/bin/echo -e "timeout -s 15 $ETIMEOUT ./fig $MODEL_FILE_K $PROPS_FILE"`

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
disown %%; wait &>/dev/null; killall sleep &>/dev/null
show " done"


## Pre-process results to build charts later on
IFUNS=("MC" "AH" "AC" "AM")  # --> changes here affect plots!
EXPERIMENTS=("${BUFFER_CAPACITIES[@]/#/k}")
RAW_RESULTS=${RESULTS}/raw_results; mkdir -p $RAW_RESULTS
MRG_RESULTS=${RESULTS}/mrg_results; mkdir -p $MRG_RESULTS
AUX_PLOTS=${RESULTS}/aux_plots;     mkdir -p $AUX_PLOTS
show -n "Merging results..."
mv $RESULTS/*.{out,err} ${RAW_RESULTS}
for k in "${BUFFER_CAPACITIES[@]}"; do
	# For tables: one file per combination of ifun and queue capacity,
	#             i.e. merge splits together
	LOG=${EXPNAME}_k${k}
	cp $RAW_RESULTS/${LOG}_MC.{out,err} $MRG_RESULTS  # MC is special, as usual
	for IFUN in "${IFUNS[@]}"; do
		if [[ ${IFUN} == "MC" ]]; then continue; fi
		cat ${RAW_RESULTS}/${LOG}_${IFUN}_s[0-9]*.out \
			>> ${MRG_RESULTS}/${LOG}"_${IFUN}.out"
 		cat ${RAW_RESULTS}/${LOG}_${IFUN}_s[0-9]*.err \
			>> ${MRG_RESULTS}/${LOG}"_${IFUN}.err"
	done
done
for s in "${SPLITS[@]}"; do
	# For plots: one file per combination of ifun and splitting,
	#            i.e. merge queues capacities together
	gather_plot_data $RAW_RESULTS EXPERIMENTS[@] "MC" 1 $CONF $PREC \
		> ${MRG_RESULTS}/MC_s1.dat  # MC is special, as usual
	for IFUN in "${IFUNS[@]}"; do
		if [[ $IFUN == "MC" ]]; then continue; fi
		gather_plot_data $RAW_RESULTS EXPERIMENTS[@] $IFUN $s $CONF $PREC \
			> ${MRG_RESULTS}/${IFUN}_s${s}.dat
	done
done
show " done"


# Build summary charts
show -n "Building tables..."
build_c_table "est"  $MRG_RESULTS EXPERIMENTS[@] IFUNS[@] SPLITS[@] $CONF $PREC \
	> $RESULTS/table_estimates.txt
build_c_table "prec" $MRG_RESULTS EXPERIMENTS[@] IFUNS[@] SPLITS[@] $CONF $PREC \
	> $RESULTS/table_precisions.txt
build_c_table "time" $MRG_RESULTS EXPERIMENTS[@] IFUNS[@] SPLITS[@] $CONF $PREC \
	> $RESULTS/table_times.txt
show " done"
#
show -n "Building plots..."
DUMMY=dummy_sizes.dat; rm -f $DUMMY
for k in "${BUFFER_CAPACITIES[@]}"; do /bin/echo -e "${k}\t0" &>> $DUMMY; done
## Plotting can't be generic since gnuplot doesn't support array variables
## Still we try our best with bash indirection
## (http://stackoverflow.com/a/8515492)
for s in "${SPLITS[@]}"; do
	# Plot per split, comparing all ifuns
	for IFUN in "${IFUNS[@]}"; do
		if [[ $IFUN == "MC" ]]; then continue; fi
		# Variables needed by this plotting script:
		eval "$IFUN=${MRG_RESULTS}/${IFUN}_s${s}.dat"
	done
	# Following must have 2+NUM_IFUNS variables defined for the gnuplot script
	gnuplot -e "
		DUMMY='${DUMMY}';
		SPLIT='${s}';
		${IFUNS[0]}='${!IFUNS[0]}';
		${IFUNS[1]}='${!IFUNS[1]}';
		${IFUNS[2]}='${!IFUNS[2]}';
		${IFUNS[3]}='${!IFUNS[3]}';
		MC='${MRG_RESULTS}/MC_s1.dat'
		" estimates_per_split.gpi
done
gs -sDEVICE=pdfwrite -dCompatibilityLevel=1.4 -dPDFSETTINGS=/default        \
	-dNOPAUSE -dQUIET -dBATCH -dDetectDuplicateImages -dCompressFonts=true  \
	-r150 -sOutputFile=plot_estimates_splits.pdf estimates_s[0-9]*.pdf
NSPLITS=${#SPLITS[*]}
DATA=($(printf "IFUN_%1d " `seq 0 $((NSPLITS-1))`))
KEYS=($(printf "NAME_%1d " `seq 0 $((NSPLITS-1))`))
for IFUN in "${IFUNS[@]}"; do
	# Plot per ifun, comparing all splits
	if [[ $IFUN == "MC" ]]; then continue; fi
	for (( i=0 ; i<NSPLITS ; i++ )); do
		# Variables needed by this plotting script:
		eval "${DATA[i]}=${MRG_RESULTS}/${IFUN}_s${SPLITS[i]}.dat"
		eval "${KEYS[i]}=split:${SPLITS[i]}"
	done
	# Following must have 2+2*NSPLITS variables defined for the gnuplot script
	gnuplot -e "
		DUMMY='${DUMMY}';
		IFUN='$IFUN';
		NFILES='$NSPLITS';
		${DATA[0]}='${!DATA[0]}';
		${DATA[1]}='${!DATA[1]}';
		${DATA[2]}='${!DATA[2]}';
		${DATA[3]}='${!DATA[3]}';
		${KEYS[0]}='${!KEYS[0]}';
		${KEYS[1]}='${!KEYS[1]}';
		${KEYS[2]}='${!KEYS[2]}';
		${KEYS[3]}='${!KEYS[3]}';
		" estimates_per_ifun.gpi;
done
shopt -s extglob  # temporarily disable shell globbing
gs -sDEVICE=pdfwrite -dCompatibilityLevel=1.4 -dPDFSETTINGS=/default        \
	-dNOPAUSE -dQUIET -dBATCH -dDetectDuplicateImages -dCompressFonts=true  \
	-r150 -sOutputFile=plot_estimates_ifuns.pdf estimates_!(s*).pdf
shopt -u extglob  # reenable shell globbing (default)
mv estimates*.pdf $AUX_PLOTS
mv plot*.pdf $RESULTS
rm $DUMMY
show "  done"


# Turn lights off
EXE_WTIME=$(format_seconds $SECONDS)  
show "Finished on $(date)"
show "Script execution walltime was $EXE_WTIME"
show "Results are in ${RESULTS}"
exit 0

