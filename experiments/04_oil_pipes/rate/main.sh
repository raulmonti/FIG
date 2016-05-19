#!/bin/bash
#
# Author:  Carlos E. Budde
# Date:    22.04.2016
# License: GPLv3
#

set -e
ECHO=`echo "/bin/echo -e"`
THIS_DIR=`readlink -f "$(dirname ${BASH_SOURCE[0]})"`


# Probe resources
source "$THIS_DIR/../../fig_utils.sh" || \
	($ECHO "[ERROR] Couldn't find fig_utils.sh" && exit 1)
if [[ "$(type -t build_fig)" != "function" ]]
then
	$ECHO "[ERROR] Bash function \"build_fig\" is undefined"
	exit 1;
elif [[ "$(type -t copy_model_file)" != "function" ]]
then
	$ECHO "[ERROR] Bash function \"copy_model_file\" is undefined"
	exit 1;
fi


# Build project
$ECHO "Building FIG"
build_fig $THIS_DIR
if [ ! -f ./fig ]; then $ECHO "[ERROR] Something went wrong"; exit 1; fi


# Prepare experiment's directory and files
$ECHO "Preparing experiments environment:"
EXP_GEN="oil_pipeline-gen.sh"
copy_model_file $EXP_GEN $THIS_DIR && \
	$ECHO "  · using model&properties generator \"$EXP_GEN\""
N=0; RESULTS="results_$N"
while [ -d $RESULTS ]; do N=$((N+1)); RESULTS="results_$N"; done
mkdir $RESULTS && unset N && \
	$ECHO "  · results will be stored in subdir \"${RESULTS}\""


# Experiments configuration
$ECHO "Configuring experiments for 60 pumps on pipeline"
declare -a E_LIFE_TIME_DISTRIB_PARAM=(0.001 0.0003 0.0001)
# for Weibull with same mean as exp: 1/0.001 = sigma*sqrt(pi/2) -> sigma = 798
declare -a W_LIFE_TIME_DISTRIB_PARAM=(798,2659.615,7978.845)
STOP_CRITERION="--stop-conf 0.80 0.4"  # Confidence coeff. and rel. precision
SPLITTINGS="--splitting 2,3,6,11"      # Splitting values for RESTART engine

#for i in {1..57}; do echo -n max\(N$(($i))f+N$(($i+1))f+N$(($i+2))f+N$(($i+3))f, ; done; for i in {1..57}; do echo -n \); done;
MAX_4="max(N1f+N2f+N3f+N4f,max(N2f+N3f+N4f+N5f,max(N3f+N4f+N5f+N6f,max(N4f+N5f+N6f+N7f,max(N5f+N6f+N7f+N8f,max(N6f+N7f+N8f+N9f,max(N7f+N8f+N9f+N10f,max(N8f+N9f+N10f+N11f,max(N9f+N10f+N11f+N12f,max(N10f+N11f+N12f+N13f,max(N11f+N12f+N13f+N14f,max(N12f+N13f+N14f+N15f,max(N13f+N14f+N15f+N16f,max(N14f+N15f+N16f+N17f,max(N15f+N16f+N17f+N18f,max(N16f+N17f+N18f+N19f,max(N17f+N18f+N19f+N20f,max(N18f+N19f+N20f+N21f,max(N19f+N20f+N21f+N22f,max(N20f+N21f+N22f+N23f,max(N21f+N22f+N23f+N24f,max(N22f+N23f+N24f+N25f,max(N23f+N24f+N25f+N26f,max(N24f+N25f+N26f+N27f,max(N25f+N26f+N27f+N28f,max(N26f+N27f+N28f+N29f,max(N27f+N28f+N29f+N30f,max(N28f+N29f+N30f+N31f,max(N29f+N30f+N31f+N32f,max(N30f+N31f+N32f+N33f,max(N31f+N32f+N33f+N34f,max(N32f+N33f+N34f+N35f,max(N33f+N34f+N35f+N36f,max(N34f+N35f+N36f+N37f,max(N35f+N36f+N37f+N38f,max(N36f+N37f+N38f+N39f,max(N37f+N38f+N39f+N40f,max(N38f+N39f+N40f+N41f,max(N39f+N40f+N41f+N42f,max(N40f+N41f+N42f+N43f,max(N41f+N42f+N43f+N44f,max(N42f+N43f+N44f+N45f,max(N43f+N44f+N45f+N46f,max(N44f+N45f+N46f+N47f,max(N45f+N46f+N47f+N48f,max(N46f+N47f+N48f+N49f,max(N47f+N48f+N49f+N50f,max(N48f+N49f+N50f+N51f,max(N49f+N50f+N51f+N52f,max(N50f+N51f+N52f+N53f,max(N51f+N52f+N53f+N54f,max(N52f+N53f+N54f+N55f,max(N53f+N54f+N55f+N56f,max(N54f+N55f+N56f+N57f,max(N55f+N56f+N57f+N58f,max(N56f+N57f+N58f+N59f,max(N57f+N58f+N59f+N60f)))))))))))))))))))))))))))))))))))))))))))))))))))))))));0;4"

#for i in {1..55}; do echo -n max\(N$(($i))f+N$(($i+1))f+N$(($i+2))f+N$(($i+3))f+N$(($i+4))f+N$(($i+5))f, ; done; for i in {1..55}; do echo -n \); done;
MAX_6="max(N1f+N2f+N3f+N4f+N5f+N6f,max(N2f+N3f+N4f+N5f+N6f+N7f,max(N3f+N4f+N5f+N6f+N7f+N8f,max(N4f+N5f+N6f+N7f+N8f+N9f,max(N5f+N6f+N7f+N8f+N9f+N10f,max(N6f+N7f+N8f+N9f+N10f+N11f,max(N7f+N8f+N9f+N10f+N11f+N12f,max(N8f+N9f+N10f+N11f+N12f+N13f,max(N9f+N10f+N11f+N12f+N13f+N14f,max(N10f+N11f+N12f+N13f+N14f+N15f,max(N11f+N12f+N13f+N14f+N15f+N16f,max(N12f+N13f+N14f+N15f+N16f+N17f,max(N13f+N14f+N15f+N16f+N17f+N18f,max(N14f+N15f+N16f+N17f+N18f+N19f,max(N15f+N16f+N17f+N18f+N19f+N20f,max(N16f+N17f+N18f+N19f+N20f+N21f,max(N17f+N18f+N19f+N20f+N21f+N22f,max(N18f+N19f+N20f+N21f+N22f+N23f,max(N19f+N20f+N21f+N22f+N23f+N24f,max(N20f+N21f+N22f+N23f+N24f+N25f,max(N21f+N22f+N23f+N24f+N25f+N26f,max(N22f+N23f+N24f+N25f+N26f+N27f,max(N23f+N24f+N25f+N26f+N27f+N28f,max(N24f+N25f+N26f+N27f+N28f+N29f,max(N25f+N26f+N27f+N28f+N29f+N30f,max(N26f+N27f+N28f+N29f+N30f+N31f,max(N27f+N28f+N29f+N30f+N31f+N32f,max(N28f+N29f+N30f+N31f+N32f+N33f,max(N29f+N30f+N31f+N32f+N33f+N34f,max(N30f+N31f+N32f+N33f+N34f+N35f,max(N31f+N32f+N33f+N34f+N35f+N36f,max(N32f+N33f+N34f+N35f+N36f+N37f,max(N33f+N34f+N35f+N36f+N37f+N38f,max(N34f+N35f+N36f+N37f+N38f+N39f,max(N35f+N36f+N37f+N38f+N39f+N40f,max(N36f+N37f+N38f+N39f+N40f+N41f,max(N37f+N38f+N39f+N40f+N41f+N42f,max(N38f+N39f+N40f+N41f+N42f+N43f,max(N39f+N40f+N41f+N42f+N43f+N44f,max(N40f+N41f+N42f+N43f+N44f+N45f,max(N41f+N42f+N43f+N44f+N45f+N46f,max(N42f+N43f+N44f+N45f+N46f+N47f,max(N43f+N44f+N45f+N46f+N47f+N48f,max(N44f+N45f+N46f+N47f+N48f+N49f,max(N45f+N46f+N47f+N48f+N49f+N50f,max(N46f+N47f+N48f+N49f+N50f+N51f,max(N47f+N48f+N49f+N50f+N51f+N52f,max(N48f+N49f+N50f+N51f+N52f+N53f,max(N49f+N50f+N51f+N52f+N53f+N54f,max(N50f+N51f+N52f+N53f+N54f+N55f,max(N51f+N52f+N53f+N54f+N55f+N56f,max(N52f+N53f+N54f+N55f+N56f+N57f,max(N53f+N54f+N55f+N56f+N57f+N58f,max(N54f+N55f+N56f+N57f+N58f+N59f,max(N55f+N56f+N57f+N58f+N59f+N60f)))))))))))))))))))))))))))))))))))))))))))))))))))))));0;6"

#"2-min(2-c11f-c12f,min(2-c21f-c22f,min(2-p11f-p12f,min(2-p21f-p22f,min(2-d11f-d12f-d13f-d14f,min(2-d21f-d22f-d23f-d24f,min(2-d31f-d32f-d33f-d34f,min(2-d41f-d42f-d43f-d44f,min(2-d51f-d52f-d53f-d54f,2-d61f-d62f-d63f-d64f)))))))));0;2"
STANDARD_MC="-e nosplit --flat $STOP_CRITERION"
RESTART_ADHOC4="--adhoc ${MAX_4} $STOP_CRITERION $SPLITTINGS -t ams"
RESTART_ADHOC6="--adhoc ${MAX_6} $STOP_CRITERION $SPLITTINGS -t ams"
RESTART_AUTO_COUPLED="--auto-coupled $STOP_CRITERION $SPLITTINGS -t ams"
RESTART_AUTO_SPLIT="--auto-split \"+\" $STOP_CRITERION $SPLITTINGS -t ams"


# Launch experiments
$ECHO "Launching experiments (for 4 consecutive broken pumps):"
for eltdp in "${E_LIFE_TIME_DISTRIB_PARAM[@]}"
do
	$ECHO -n "  · for mean failure times ~ $mft..."

	# Generate model and properties files to fit this experiment
	MODEL_FILE=oil_pipes_4_eltdp${eltdp}.sa
	PROPS_FILE=oil_pipes_4_eltdp${eltdp}.pp
	LOG=${RESULTS}/oil_pipes_4_eltdp${eltdp}
	bash $EXP_GEN 60 4 0 'E' $eltdp 1>$MODEL_FILE 2>$PROPS_FILE
	EXE=`$ECHO "timeout -s 15 10h ./fig $MODEL_FILE $PROPS_FILE"`
  
	# Standard Monte Carlo
	poll_till_free; $ECHO -n " MC"
	$EXE $STANDARD_MC 1>>${LOG}"_MC.out" 2>>${LOG}"_MC.err" &

	# RESTART with ad hoc
	poll_till_free; $ECHO -n ", AH"
	$EXE $RESTART_ADHOC4 1>>${LOG}"_AH.out" 2>>${LOG}"_AH.err" &

	# RESTART with auto-split
	poll_till_free; $ECHO -n ", AS"
	$EXE $RESTART_AUTO_SPLIT 1>>${LOG}"_AS.out" 2>>${LOG}"_AS.err" &

	# RESTART with auto-coupled experiments are omitted
	# since the importance vector wouldn't fit in memory

	$ECHO "... done"
done

for wltdp in "${W_LIFE_TIME_DISTRIB_PARAM[@]}"
do
	$ECHO -n "  · for mean failure times ~ $wltdp..."

	# Generate model and properties files to fit this experiment
	MODEL_FILE=oil_pipes_4_wltdp${wltdp}.sa
	PROPS_FILE=oil_pipes_4_wltdp${wltdp}.pp
	LOG=${RESULTS}/oil_pipes_4_wltdp${wltdp}
	bash $EXP_GEN 60 4 0 'W' $wltdp 1>$MODEL_FILE 2>$PROPS_FILE
	EXE=`$ECHO "timeout -s 15 10h ./fig $MODEL_FILE $PROPS_FILE"`
  
	# Standard Monte Carlo
	poll_till_free; $ECHO -n " MC"
	$EXE $STANDARD_MC 1>>${LOG}"_MC.out" 2>>${LOG}"_MC.err" &

	# RESTART with ad hoc
	poll_till_free; $ECHO -n ", AH"
	$EXE $RESTART_ADHOC4 1>>${LOG}"_AH.out" 2>>${LOG}"_AH.err" &

	# RESTART with auto-split
	poll_till_free; $ECHO -n ", AS"
	$EXE $RESTART_AUTO_SPLIT 1>>${LOG}"_AS.out" 2>>${LOG}"_AS.err" &

	# RESTART with auto-coupled experiments are omitted
	# since the importance vector wouldn't fit in memory

	$ECHO "... done"
done

$ECHO "Launching experiments (for 6 consecutive broken pumps):"
for eltdp in "${E_LIFE_TIME_DISTRIB_PARAM[@]}"
do
	$ECHO -n "  · for mean failure times ~ $mft..."

	# Generate model and properties files to fit this experiment
	MODEL_FILE=oil_pipes_6_eltdp${eltdp}.sa
	PROPS_FILE=oil_pipes_6_eltdp${eltdp}.pp
	LOG=${RESULTS}/oil_pipes_6_eltdp${eltdp}
	bash $EXP_GEN 60 6 0 'E' $eltdp 1>$MODEL_FILE 2>$PROPS_FILE
	EXE=`$ECHO "timeout -s 15 10h ./fig $MODEL_FILE $PROPS_FILE"`
  
	# Standard Monte Carlo
	poll_till_free; $ECHO -n " MC"
	$EXE $STANDARD_MC 1>>${LOG}"_MC.out" 2>>${LOG}"_MC.err" &

	# RESTART with ad hoc
	poll_till_free; $ECHO -n ", AH"
	$EXE $RESTART_ADHOC6 1>>${LOG}"_AH.out" 2>>${LOG}"_AH.err" &

	# RESTART with auto-split
	poll_till_free; $ECHO -n ", AS"
	$EXE $RESTART_AUTO_SPLIT 1>>${LOG}"_AS.out" 2>>${LOG}"_AS.err" &

	# RESTART with auto-coupled experiments are omitted
	# since the importance vector wouldn't fit in memory

	$ECHO "... done"
done

for wltdp in "${W_LIFE_TIME_DISTRIB_PARAM[@]}"
do
	$ECHO -n "  · for mean failure times ~ $wltdp..."

	# Generate model and properties files to fit this experiment
	MODEL_FILE=oil_pipes_6_wltdp${wltdp}.sa
	PROPS_FILE=oil_pipes_6_wltdp${wltdp}.pp
	LOG=${RESULTS}/oil_pipes_6_wltdp${wltdp}
	bash $EXP_GEN 60 6 0 'W' $wltdp 1>$MODEL_FILE 2>$PROPS_FILE
	EXE=`$ECHO "timeout -s 15 10h ./fig $MODEL_FILE $PROPS_FILE"`
  
	# Standard Monte Carlo
	poll_till_free; $ECHO -n " MC"
	$EXE $STANDARD_MC 1>>${LOG}"_MC.out" 2>>${LOG}"_MC.err" &

	# RESTART with ad hoc
	poll_till_free; $ECHO -n ", AH"
	$EXE $RESTART_ADHOC6 1>>${LOG}"_AH.out" 2>>${LOG}"_AH.err" &

	# RESTART with auto-split
	poll_till_free; $ECHO -n ", AS"
	$EXE $RESTART_AUTO_SPLIT 1>>${LOG}"_AS.out" 2>>${LOG}"_AS.err" &

	# RESTART with auto-coupled experiments are omitted
	# since the importance vector wouldn't fit in memory

	$ECHO "... done"
done

# Wait till termination
$ECHO "Waiting for all experiments to finish"
declare -a A=(. :) ; N=0
RUNNING=`$ECHO "pgrep -u $(whoami) fig"`
while [ -n "`$RUNNING`" ]; do $ECHO -n ${A[$N]}; N=$(((N+1)%2)); sleep 9; done
unset A N
$ECHO
$ECHO "All experiments finished"


exit 0

