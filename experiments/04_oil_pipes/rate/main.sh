#!/bin/bash
#
# Author:  Raúl E. Monti
# Date:    19.05.2016
# Update:  Carlos E. Budde @ 23.05.2016
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
EXP_GEN="oil_pipeline-gen.sh"
copy_model_file $EXP_GEN $THIS_DIR && \
	show "  · using model&properties generator \"$EXP_GEN\""
N=0; RESULTS="results_$N"
while [ -d $RESULTS ]; do N=$((N+1)); RESULTS="results_$N"; done
mkdir $RESULTS && unset N && \
	show "  · results will be stored in subdir \"${RESULTS}\""


# Experiments configuration
show "Configuring experiments for 60 pumps on pipeline"
declare -a E_LIFE_TIME_DISTRIB_PARAM=(0.001 0.0003 0.0001)
# for Weibull with same mean as exp: 1/0.001 = sigma*sqrt(pi/2) -> sigma = 798
declare -a W_LIFE_TIME_DISTRIB_PARAM=(798 2659.615 7978.845)
STOP_CRITERION="--stop-conf 0.80 0.4"  # Confidence coeff. and rel. precision
SPLITTINGS="--splitting 2,3,6"      # Splitting values for RESTART engine

#for i in {1..57}; do echo -n max\(N$(($i))+N$(($i+1))+N$(($i+2))+N$(($i+3)), ; done; for i in {1..57}; do echo -n \); done; echo ";0;4";
MAX_4="max(N1+N2+N3+N4,max(N2+N3+N4+N5,max(N3+N4+N5+N6,max(N4+N5+N6+N7,max(N5+N6+N7+N8,max(N6+N7+N8+N9,max(N7+N8+N9+N10,max(N8+N9+N10+N11,max(N9+N10+N11+N12,max(N10+N11+N12+N13,max(N11+N12+N13+N14,max(N12+N13+N14+N15,max(N13+N14+N15+N16,max(N14+N15+N16+N17,max(N15+N16+N17+N18,max(N16+N17+N18+N19,max(N17+N18+N19+N20,max(N18+N19+N20+N21,max(N19+N20+N21+N22,max(N20+N21+N22+N23,max(N21+N22+N23+N24,max(N22+N23+N24+N25,max(N23+N24+N25+N26,max(N24+N25+N26+N27,max(N25+N26+N27+N28,max(N26+N27+N28+N29,max(N27+N28+N29+N30,max(N28+N29+N30+N31,max(N29+N30+N31+N32,max(N30+N31+N32+N33,max(N31+N32+N33+N34,max(N32+N33+N34+N35,max(N33+N34+N35+N36,max(N34+N35+N36+N37,max(N35+N36+N37+N38,max(N36+N37+N38+N39,max(N37+N38+N39+N40,max(N38+N39+N40+N41,max(N39+N40+N41+N42,max(N40+N41+N42+N43,max(N41+N42+N43+N44,max(N42+N43+N44+N45,max(N43+N44+N45+N46,max(N44+N45+N46+N47,max(N45+N46+N47+N48,max(N46+N47+N48+N49,max(N47+N48+N49+N50,max(N48+N49+N50+N51,max(N49+N50+N51+N52,max(N50+N51+N52+N53,max(N51+N52+N53+N54,max(N52+N53+N54+N55,max(N53+N54+N55+N56,max(N54+N55+N56+N57,max(N55+N56+N57+N58,max(N56+N57+N58+N59,N57+N58+N59+N60))))))))))))))))))))))))))))))))))))))))))))))))))))))));0;4"

#for i in {1..55}; do echo -n max\(N$(($i))+N$(($i+1))+N$(($i+2))+N$(($i+3))+N$(($i+4))+N$(($i+5)), ; done; for i in {1..55}; do echo -n \); done; echo ";0;6";
MAX_6="max(N1+N2+N3+N4+N5+N6,max(N2+N3+N4+N5+N6+N7,max(N3+N4+N5+N6+N7+N8,max(N4+N5+N6+N7+N8+N9,max(N5+N6+N7+N8+N9+N10,max(N6+N7+N8+N9+N10+N11,max(N7+N8+N9+N10+N11+N12,max(N8+N9+N10+N11+N12+N13,max(N9+N10+N11+N12+N13+N14,max(N10+N11+N12+N13+N14+N15,max(N11+N12+N13+N14+N15+N16,max(N12+N13+N14+N15+N16+N17,max(N13+N14+N15+N16+N17+N18,max(N14+N15+N16+N17+N18+N19,max(N15+N16+N17+N18+N19+N20,max(N16+N17+N18+N19+N20+N21,max(N17+N18+N19+N20+N21+N22,max(N18+N19+N20+N21+N22+N23,max(N19+N20+N21+N22+N23+N24,max(N20+N21+N22+N23+N24+N25,max(N21+N22+N23+N24+N25+N26,max(N22+N23+N24+N25+N26+N27,max(N23+N24+N25+N26+N27+N28,max(N24+N25+N26+N27+N28+N29,max(N25+N26+N27+N28+N29+N30,max(N26+N27+N28+N29+N30+N31,max(N27+N28+N29+N30+N31+N32,max(N28+N29+N30+N31+N32+N33,max(N29+N30+N31+N32+N33+N34,max(N30+N31+N32+N33+N34+N35,max(N31+N32+N33+N34+N35+N36,max(N32+N33+N34+N35+N36+N37,max(N33+N34+N35+N36+N37+N38,max(N34+N35+N36+N37+N38+N39,max(N35+N36+N37+N38+N39+N40,max(N36+N37+N38+N39+N40+N41,max(N37+N38+N39+N40+N41+N42,max(N38+N39+N40+N41+N42+N43,max(N39+N40+N41+N42+N43+N44,max(N40+N41+N42+N43+N44+N45,max(N41+N42+N43+N44+N45+N46,max(N42+N43+N44+N45+N46+N47,max(N43+N44+N45+N46+N47+N48,max(N44+N45+N46+N47+N48+N49,max(N45+N46+N47+N48+N49+N50,max(N46+N47+N48+N49+N50+N51,max(N47+N48+N49+N50+N51+N52,max(N48+N49+N50+N51+N52+N53,max(N49+N50+N51+N52+N53+N54,max(N50+N51+N52+N53+N54+N55,max(N51+N52+N53+N54+N55+N56,max(N52+N53+N54+N55+N56+N57,max(N53+N54+N55+N56+N57+N58,max(N54+N55+N56+N57+N58+N59,N55+N56+N57+N58+N59+N60))))))))))))))))))))))))))))))))))))))))))))))))))))));0;6"

STANDARD_MC="-e nosplit --flat $STOP_CRITERION"
RESTART_ADHOC4="--adhoc ${MAX_4} $STOP_CRITERION $SPLITTINGS"
RESTART_ADHOC6="--adhoc ${MAX_6} $STOP_CRITERION $SPLITTINGS"
RESTART_ACOMP="--acomp \"+\" $STOP_CRITERION $SPLITTINGS"

# Launch experiments
show "Launching experiments (for 4 consecutive broken pumps):"
for eltdp in "${E_LIFE_TIME_DISTRIB_PARAM[@]}"
do
	show -n "  · for 4-out-of-60:F with Exponentials..."

	# Generate model and properties files to fit this experiment
	MODEL_FILE=oil_pipes_4_eltdp${eltdp}.sa
	PROPS_FILE=oil_pipes_4_eltdp${eltdp}.pp
	LOG=${RESULTS}/oil_pipes_4_eltdp${eltdp}
	bash $EXP_GEN 60 4 0 'E' $eltdp 1>$MODEL_FILE 2>$PROPS_FILE
	EXE=`/bin/echo -e "timeout -s 15 16h ./fig $MODEL_FILE $PROPS_FILE"`

	# RESTART with monolithic (auto ifun) experiments are omitted
	# since the importance vector wouldn't fit in memory

	# RESTART with compositional (auto ifun)
	poll_till_free; show -n " AC"
	$EXE $RESTART_ACOMP 1>>${LOG}"_AC.out" 2>>${LOG}"_AC.err" &

	# RESTART with ad hoc
	poll_till_free; show -n ", AH"
	$EXE $RESTART_ADHOC4 1>>${LOG}"_AH.out" 2>>${LOG}"_AH.err" &
  
	# Standard Monte Carlo
	poll_till_free; show -n ", MC"
	$EXE $STANDARD_MC 1>>${LOG}"_MC.out" 2>>${LOG}"_MC.err" &

	show "... done"
done

for wltdp in "${W_LIFE_TIME_DISTRIB_PARAM[@]}"
do
	show -n "  · for 4-out-of-60:F with Rayleighs..."

	# Generate model and properties files to fit this experiment
	MODEL_FILE=oil_pipes_4_wltdp${wltdp}.sa
	PROPS_FILE=oil_pipes_4_wltdp${wltdp}.pp
	LOG=${RESULTS}/oil_pipes_4_wltdp${wltdp}
	bash $EXP_GEN 60 4 0 'W' $wltdp 1>$MODEL_FILE 2>$PROPS_FILE
	EXE=`/bin/echo -e "timeout -s 15 16h ./fig $MODEL_FILE $PROPS_FILE"`

	# RESTART with monolithic (auto ifun) experiments are omitted
	# since the importance vector wouldn't fit in memory

	# RESTART with compositional (auto ifun)
	poll_till_free; show -n " AC"
	$EXE $RESTART_ACOMP 1>>${LOG}"_AC.out" 2>>${LOG}"_AC.err" &

	# RESTART with ad hoc
	poll_till_free; show -n ", AH"
	$EXE $RESTART_ADHOC4 1>>${LOG}"_AH.out" 2>>${LOG}"_AH.err" &
  
	# Standard Monte Carlo
	poll_till_free; show -n ", MC"
	$EXE $STANDARD_MC 1>>${LOG}"_MC.out" 2>>${LOG}"_MC.err" &

	show "... done"
done

show "Launching experiments (for 6 consecutive broken pumps):"
for eltdp in "${E_LIFE_TIME_DISTRIB_PARAM[@]}"
do
	show -n "  · for 6-out-of-60:F with Exponentials..."

	# Generate model and properties files to fit this experiment
	MODEL_FILE=oil_pipes_6_eltdp${eltdp}.sa
	PROPS_FILE=oil_pipes_6_eltdp${eltdp}.pp
	LOG=${RESULTS}/oil_pipes_6_eltdp${eltdp}
	bash $EXP_GEN 60 6 0 'E' $eltdp 1>$MODEL_FILE 2>$PROPS_FILE
	EXE=`/bin/echo -e "timeout -s 15 16h ./fig $MODEL_FILE $PROPS_FILE"`

	# RESTART with monolithic (auto ifun) experiments are omitted
	# since the importance vector wouldn't fit in memory

	# RESTART with compositional (auto ifun)
	poll_till_free; show -n " AC"
	$EXE $RESTART_ACOMP 1>>${LOG}"_AC.out" 2>>${LOG}"_AC.err" &

	# RESTART with ad hoc
	poll_till_free; show -n ", AH"
	$EXE $RESTART_ADHOC6 1>>${LOG}"_AH.out" 2>>${LOG}"_AH.err" &
  
	# Standard Monte Carlo
	poll_till_free; show -n ", MC"
	$EXE $STANDARD_MC 1>>${LOG}"_MC.out" 2>>${LOG}"_MC.err" &

	show "... done"
done

for wltdp in "${W_LIFE_TIME_DISTRIB_PARAM[@]}"
do
	show -n "  · for 6-out-of-60:F with Rayleighs..."

	# Generate model and properties files to fit this experiment
	MODEL_FILE=oil_pipes_6_wltdp${wltdp}.sa
	PROPS_FILE=oil_pipes_6_wltdp${wltdp}.pp
	LOG=${RESULTS}/oil_pipes_6_wltdp${wltdp}
	bash $EXP_GEN 60 6 0 'W' $wltdp 1>$MODEL_FILE 2>$PROPS_FILE
	EXE=`/bin/echo -e "timeout -s 15 16h ./fig $MODEL_FILE $PROPS_FILE"`

	# RESTART with monolithic (auto ifun) experiments are omitted
	# since the importance vector wouldn't fit in memory

	# RESTART with compositional (auto ifun)
	poll_till_free; show -n " AC"
	$EXE $RESTART_ACOMP 1>>${LOG}"_AC.out" 2>>${LOG}"_AC.err" &

	# RESTART with ad hoc
	poll_till_free; show -n ", AH"
	$EXE $RESTART_ADHOC6 1>>${LOG}"_AH.out" 2>>${LOG}"_AH.err" &
  
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

