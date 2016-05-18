#!/bin/bash
#
# Author:  Carlos E. Budde
# Date:    22.04.2016
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
EXP_GEN="database-gen.sh"
copy_model_file $EXP_GEN $THIS_DIR && \
	show "  · using model&properties generator \"$EXP_GEN\""
N=0; RESULTS="results_$N"
while [ -d $RESULTS ]; do N=$((N+1)); RESULTS="results_$N"; done
mkdir $RESULTS && unset N && \
	show "  · results will be stored in subdir \"${RESULTS}\""


# Experiments configuration
show "Configuring experiments for 2 'RED'undancy,"
show "                            6 'D'isk clusters,"
show "                            2 'C'ontroller types,"
show "                            2 'P'rocessors types"
declare -a MEAN_FAILURE_TIMES=(2000 8000 32000 128000)
STOP_CRITERION="--stop-conf 0.90 0.3"  # Confidence coeff. and rel. precision
SPLITTINGS="--splitting 3,6,11"        # Splitting values for RESTART engine
MIN_OC="2-min(2-c11f-c12f,min(2-c21f-c22f,min(2-p11f-p12f,min(2-p21f-p22f,min(2-d11f-d12f-d13f-d14f,min(2-d21f-d22f-d23f-d24f,min(2-d31f-d32f-d33f-d34f,min(2-d41f-d42f-d43f-d44f,min(2-d51f-d52f-d53f-d54f,2-d61f-d62f-d63f-d64f)))))))));0;2"
COMP_FUN1="\"+\""
COMP_FUN2="'(Disk11*Disk12*Disk13*Disk14*Disk21*Disk22*Disk23*Disk24*Disk31*Disk32*Disk33*Disk34*Disk41*Disk42*Disk43*Disk44*Disk51*Disk52*Disk53*Disk54*Disk61*Disk62*Disk63*Disk64)+(Controller11*Controller12*Controller21*Controller22)+(Processor11*Processor12*Processor21*Processor22);3;16777248;1'"
COMP_FUN3="'(Disk11*Disk12)+(Disk11*Disk13)+(Disk11*Disk14)+(Disk12*Disk13)+(Disk12*Disk14)+(Disk13*Disk14)+(Disk21*Disk22)+(Disk21*Disk23)+(Disk21*Disk24)+(Disk22*Disk23)+(Disk22*Disk24)+(Disk23*Disk24)+(Disk31*Disk32)+(Disk31*Disk33)+(Disk31*Disk34)+(Disk32*Disk33)+(Disk32*Disk34)+(Disk33*Disk34)+(Disk41*Disk42)+(Disk41*Disk43)+(Disk41*Disk44)+(Disk42*Disk43)+(Disk42*Disk44)+(Disk43*Disk44)+(Disk51*Disk52)+(Disk51*Disk53)+(Disk51*Disk54)+(Disk52*Disk53)+(Disk52*Disk54)+(Disk53*Disk54)+(Disk61*Disk62)+(Disk61*Disk63)+(Disk61*Disk64)+(Disk62*Disk63)+(Disk62*Disk64)+(Disk63*Disk64)+(Controller11*Controller12)+(Controller21*Controller22)+(Processor11*Processor12)+(Processor21*Processor22);0;160;1'"
STANDARD_MC="-e nosplit --flat $STOP_CRITERION"
RESTART_ADHOC="--adhoc $MIN_OC $STOP_CRITERION $SPLITTINGS -t fix"
RESTART_ACOMP1="--acomp $COMP_FUN1 $STOP_CRITERION $SPLITTINGS -t fix"
RESTART_ACOMP2="--acomp-exp $COMP_FUN2 $STOP_CRITERION $SPLITTINGS -t fix"
RESTART_ACOMP3="--acomp-exp $COMP_FUN3 $STOP_CRITERION $SPLITTINGS -t fix"


# Launch experiments
show "Launching experiments:"
for mft in "${MEAN_FAILURE_TIMES[@]}"
do
	show -n "  · for mean failure times ~ $mft..."

	# Generate model and properties files to fit this experiment
	MODEL_FILE=database_mft${mft}.sa
	PROPS_FILE=database_mft${mft}.pp
	LOG=${RESULTS}/database_mft${mft}
	bash $EXP_GEN 2 6 2 2 $mft 1>$MODEL_FILE 2>$PROPS_FILE
	EXE=`/bin/echo -e "timeout -s 15 18h ./fig $MODEL_FILE $PROPS_FILE"`

	# RESTART with --amono experiments are omitted
	# since the importance vector wouldn't fit in memory

	# RESTART with --acomp, version 1
	poll_till_free; show -n " AC1"
	$EXE $RESTART_ACOMP1 1>>${LOG}"_AC1.out" 2>>${LOG}"_AC1.err" &

	# RESTART with --acomp, version 2
	poll_till_free; show -n ", AC2"
	$EXE $RESTART_ACOMP2 1>>${LOG}"_AC2.out" 2>>${LOG}"_AC2.err" &

	# RESTART with --acomp, version 3
	poll_till_free; show -n ", AC3"
	$EXE $RESTART_ACOMP3 1>>${LOG}"_AC3.out" 2>>${LOG}"_AC3.err" &

	# RESTART with ad hoc
	poll_till_free; show -n ", AH"
	$EXE $RESTART_ADHOC 1>>${LOG}"_AH.out" 2>>${LOG}"_AH.err" &
  
	# Standard Monte Carlo
	poll_till_free; show -n ", MC"
	$EXE $STANDARD_MC 1>>${LOG}"_MC.out" 2>>${LOG}"_MC.err" &

	show "... done"
done


# Wait till termination
show -n "Waiting for all experiments to finish..."
wait
show " done"
EXE_WTIME=$(format_seconds $SECONDS)  
show "Script execution walltime was $EXE_WTIME"
show "Results are in ${RESULTS}"


exit 0

