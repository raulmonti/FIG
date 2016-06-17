#!/bin/bash
#
# Author:  Carlos E. Budde
# Date:    22.04.2016
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
EXP_GEN="database-gen.sh"
copy_model_file $EXP_GEN $CWD && \
	show "  · using model&properties generator \"$EXP_GEN\""
N=0; RESULTS="results_$N"
while [ -d $RESULTS ]; do N=$((N+1)); RESULTS="results_$N"; done
mkdir $RESULTS && unset N && \
	show "  · results will be stored in subdir \"${RESULTS}\""


# Experiments configuration
TO="4h"
CONF=0.9  # Confidence coefficient
PREC=0.2  # Relative precision
SPLITS=(2 3 6 11)  # RESTART splittings to test
REDUNDANCY=(2 3 4)
NDC=6     # Number of disk clusters
NCT=2     # Number of controller types
NPT=2     # Number of processor types
MFT=2000  # Basic mean failure time
EXPNAME="database"
#
show "Configuring experiments for $NDC disk clusters,"
show "                            $NCT controller types,"
show "                            $NPT processor types,"
show "                            $MFT basic MFT"
SPLITTING="--splitting "
for S in "${SPLITS[@]}"; do SPLITTING+="$S,"; done; SPLITTING="${SPLITTING%,}"
STOP_CRITERION="--stop-conf $CONF $PREC"
ETIMEOUT="${TO##*[0-9]}"  # Timeout per experiment (one ifun, all splits)
ETIMEOUT=$(bc <<< "${TO%%[a-z]*}*${#SPLITS[@]}*2")"$ETIMEOUT"
show "Timeouts: $TO per split; $ETIMEOUT per experiment"
source ifuns.sh  # Ad hoc / compositional importance functions constructors

echo
min_num_oc 2 $NDC $NCT $NPT
echo
echo
min_num_oc 3 $NDC $NCT $NPT
echo
echo
comp_fun_coarse 2 $NDC $NCT $NPT
echo
echo
comp_fun_coarse 3 $NDC $NCT $NPT
echo
echo
comp_fun_med 2 $NDC $NCT $NPT
echo
echo
comp_fun_med 3 $NDC $NCT $NPT
echo
echo
comp_fun_fine 2 $NDC $NCT $NPT
echo
echo
comp_fun_fine 3 $NDC $NCT $NPT
echo

exit 1

## MIN_OC="2-min(2-c11f-c12f,min(2-c21f-c22f,min(2-p11f-p12f,min(2-p21f-p22f,min(2-d11f-d12f-d13f-d14f,min(2-d21f-d22f-d23f-d24f,min(2-d31f-d32f-d33f-d34f,min(2-d41f-d42f-d43f-d44f,min(2-d51f-d52f-d53f-d54f,2-d61f-d62f-d63f-d64f)))))))));0;2"
## COMP_FUN1="\"+\""
## COMP_FUN2="'(Disk11*Disk12*Disk13*Disk14*Disk21*Disk22*Disk23*Disk24*Disk31*Disk32*Disk33*Disk34*Disk41*Disk42*Disk43*Disk44*Disk51*Disk52*Disk53*Disk54*Disk61*Disk62*Disk63*Disk64)+(Controller11*Controller12*Controller21*Controller22)+(Processor11*Processor12*Processor21*Processor22);3;16777248;1'"
## COMP_FUN3="'(Disk11*Disk12)+(Disk11*Disk13)+(Disk11*Disk14)+(Disk12*Disk13)+(Disk12*Disk14)+(Disk13*Disk14)+(Disk21*Disk22)+(Disk21*Disk23)+(Disk21*Disk24)+(Disk22*Disk23)+(Disk22*Disk24)+(Disk23*Disk24)+(Disk31*Disk32)+(Disk31*Disk33)+(Disk31*Disk34)+(Disk32*Disk33)+(Disk32*Disk34)+(Disk33*Disk34)+(Disk41*Disk42)+(Disk41*Disk43)+(Disk41*Disk44)+(Disk42*Disk43)+(Disk42*Disk44)+(Disk43*Disk44)+(Disk51*Disk52)+(Disk51*Disk53)+(Disk51*Disk54)+(Disk52*Disk53)+(Disk52*Disk54)+(Disk53*Disk54)+(Disk61*Disk62)+(Disk61*Disk63)+(Disk61*Disk64)+(Disk62*Disk63)+(Disk62*Disk64)+(Disk63*Disk64)+(Controller11*Controller12)+(Controller21*Controller22)+(Processor11*Processor12)+(Processor21*Processor22);40;160;1'"
## COMP_FUN4="'(Disk11*Disk12*Disk13*Disk14)+(Disk21*Disk22*Disk23*Disk24)+(Disk31*Disk32*Disk33*Disk34)+(Disk41*Disk42*Disk43*Disk44)+(Disk51*Disk52*Disk53*Disk54)+(Disk61*Disk62*Disk63*Disk64)+(Controller11*Controller12)+(Controller21*Controller22)+(Processor11*Processor12)+(Processor21*Processor22);10;112;1'"
## STANDARD_MC="-e nosplit --flat $STOP_CRITERION --timeout $TO"
## RESTART_ADHOC="--adhoc $MIN_OC $STOP_CRITERION $SPLITTING --timeout $TO"
## RESTART_ACOMP1="--acomp $COMP_FUN1 $STOP_CRITERION $SPLITTING --timeout $TO"
## RESTART_ACOMP2="--acomp $COMP_FUN2 $STOP_CRITERION $SPLITTING --timeout $TO --post-process exp 2"
## RESTART_ACOMP3="--acomp $COMP_FUN3 $STOP_CRITERION $SPLITTING --timeout $TO --post-process exp 2"
## RESTART_ACOMP4="--acomp $COMP_FUN4 $STOP_CRITERION $SPLITTING --timeout $TO --post-process exp 2"


# Launch experiments
show "Launching experiments:"
for R in "${REDUNDANCY[@]}"
do
	show -n "  · for redundacy $R..."

	# Generate model and properties files to fit this experiment
	MODEL_FILE=database_r${R}.sa
	PROPS_FILE=database_r${R}.pp
	LOG=${RESULTS}/database_r${R}
	bash $EXP_GEN $R $NDC $NCT $NPT $MFT 1>$MODEL_FILE 2>$PROPS_FILE
	EXE=`/bin/echo -e "timeout -s 15 $ETIMEOUT ./fig $MODEL_FILE $PROPS_FILE"`

	# RESTART with monolithic (auto ifun) experiments are omitted
	# since the importance vector wouldn't fit in memory

	# RESTART with compositional (auto ifun), version 1
	poll_till_free $EXPNAME; show -n " AC1"
	$EXE $RESTART_ACOMP1 1>>${LOG}"_AC1.out" 2>>${LOG}"_AC1.err" &

	# RESTART with compositional (auto ifun), version 2
	poll_till_free $EXPNAME; show -n ", AC2"
	$EXE $RESTART_ACOMP2 1>>${LOG}"_AC2.out" 2>>${LOG}"_AC2.err" &

	# RESTART with compositional (auto ifun), version 3
	poll_till_free $EXPNAME; show -n ", AC3"
	$EXE $RESTART_ACOMP3 1>>${LOG}"_AC3.out" 2>>${LOG}"_AC3.err" &

	# RESTART with compositional (auto ifun), version 4
	poll_till_free $EXPNAME; show -n ", AC4"
	$EXE $RESTART_ACOMP4 1>>${LOG}"_AC4.out" 2>>${LOG}"_AC4.err" &

	# RESTART with ad hoc
	poll_till_free $EXPNAME; show -n ", AH"
	$EXE $RESTART_ADHOC 1>>${LOG}"_AH.out" 2>>${LOG}"_AH.err" &
  
	# Standard Monte Carlo
	poll_till_free $EXPNAME; show -n ", MC"
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


# Build summary charts
show -n "Building tables..."
IFUNS=("MC" "AH" "AC1" "AC2" "AC3" "AC4")
EXPERIMENTS=("${REDUNDANCY[@]/#/r}")
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

