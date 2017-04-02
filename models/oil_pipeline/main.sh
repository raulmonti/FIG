#!/bin/bash
# Calling arguments:
#  $1: number of pipes
#  $2: consecutive broken pipes
#  $3: fail distribution
#  $4: repair distribution
#  $5: output file name for model (and properties)


# Check calling arguments
if [ $# -ne 5 ]; then
	echo "[ERROR] Invalid invocation, call with 5 arguments:"
	echo "  1. Number of pipes                       (aka 'N', e.g. 60)"
	echo "  2. Number of consecutive broken pipes    (aka 'K', e.g.  4)"
	echo "  3. Fail distribution             (e.g. exponential(0.001) )"
	echo "  4. Repair distribution           (e.g. lognormal(1.21,.64))"
	echo "  5. Output filename               (e.g. \"oil_pipes_60_4.sa\")"
	exit 1
fi


# INTRO
echo "/*" > $5  # NOTE: clears model file, if any
echo " * Non-Markov consecutive-k-out-of-n:F repairable system for the FIG tool" >> $5
echo " * Budde, Monti, Rodríguez, D'Argenio | 2017" >> $5
echo " *" >> $5
echo " *{-" >> $5
echo " * Concept of the Non-Markov consecutive-k-out-of-n:F repairable system:" >> $5
echo " *" >> $5
echo " * ---> (P1) ---> (P2) ---> ··· ---> (Pn) --->" >> $5
echo " *" >> $5
echo " * For a graphical illustration imagine an oil pipeline with 'n' equally-spaced" >> $5
echo " * pressure pumps along it, {P1..Pn}. Each pump sends oil to the next and it can" >> $5
echo " * only fail and get repaired (viz. there are just two possible states {up,down})" >> $5
echo " * The whole pipeline \"fails\" as soon as 'k' (or more) consecutive pumps fail." >> $5
echo " * A single repairman attends one broken pump at a time and fixes it." >> $5
echo " *" >> $5
echo " * In this implementation the probability of failure of each pump will be " >> $5
echo " * independent of the system's general state; the (k-1)-step Markov dependence " >> $5
echo " * that some authors assume is thus dropped. Also the repairman will apply a" >> $5
echo " * static repair policy: it will always choose the first failed pump, that is," >> $5
echo " * the broken pump with smallest index in {P1..Pn}." >> $5
echo " *" >> $5
echo " * Initial state: all pumps are up." >> $5
echo " * Reference event: any system transition." >> $5
echo " * Rare event: system failure caused by at least 'k' consecutive failed pumps" >> $5
echo " *-}" >> $5
echo " */" >> $5

# PARAMETERS IN THE LITERATURE
echo "" >> $5
echo "// The following distributions are used in page 252, Section 4.1," >> $5
echo "// \"System steady-state unavailability and MTBF\" of José Villén-Altamirano" >> $5
echo "// \"RESTART simulation of non-Markov consecutive-k-out-of-n:F repairable" >> $5
echo "// systems\", Reliability Engineering and System Safety, Vol. 95, Issue 3," >> $5
echo "// March 2010, pp. 247-254:" >> $5
echo "//  · Repair time ~ Log-normal(1.21,0.8)" >> $5
echo "//  · Nodes lifetime ~ Exponential(lambda) or Rayleigh(sigma)" >> $5
echo "//                     for (lambda,sigma) in {(0.001 ,  798.0  )," >> $5
echo "//                                            (0.0003, 2659.615)," >> $5
echo "//                                            (0.0001, 7978.845)}" >> $5
echo "// NOTE: Rayleigh(s) distribution is actually" >> $5
echo "//       a Weibull distribution with shape == 2 and rate == s*sqrt(2)" >> $5
echo -en "\n\n" >> $5


# MACROS
BASE_DIR=$(dirname `readlink -f ${BASH_SOURCE[0]}`)
if [ ! -d $BASE_DIR ]; then
	echo "[ERROR] Unable to locate the base directory"
	echo "        of the \"${BASH_SOURCE[0]}\" script"
	exit 1
fi
BASIC_EVENT="$BASE_DIR/be.sh"
AND_GATE="$BASE_DIR/and.sh"
OR_GATE="$BASE_DIR/or.sh"
REPAIRMAN="$BASE_DIR/repman.sh"
END=$((${1}-${2}+1))
## echo "pwd:"
## pwd
## echo "readlink:"
## echo "$(dirname `readlink -f ${BASH_SOURCE[0]}`)"
## exit 1

# BUILD PIPES (aka Basic Events)
for i in $(seq 1 ${1}); do
	bash $BASIC_EVENT pipe$i $3 $4 >> $5;
	echo -en "\n\n" >> $5;
done

# BUILD ANDS
#for i in $(seq 1 $END); do 
#    bash $AND_GATE and$i 6 "pipe$((${i})) pipe$((${i}+1)) pipe$((${i}+2)) pipe$((${i}+3))"  >> $2 ;
#done

# BUILD THE REPAIRMAN
#bes=""
#for i in $(seq 1 ${1}); do
#    bes=${bes}" pipe$i"
#done
bes=`printf " pipe%d" $(seq 1 $1)`
bash $REPAIRMAN "A" ${1} "$bes" >> $5;

# BUILD THE OR TOP NODE
#ands=""
#for i in $(seq 1 $END); do
#    ands=${ands}" and$i"
#done
#bash $OR_GATE "pipeline" $END "$ands" >> $2;

##  BUILD SYSTEM NODE
## echo "module System" >> $2;
## echo "    sys_broken: bool init false;" >> $2;
## echo "    [fail_pipeline??] -> (sys_broken'=true);" >> $2;
## echo "    [repair_pipeline??] -> (sys_broken'=false);" >> $2;
## echo "endmodule" >> $2;

# BUILD PROPERTIES
echo -e "\n\nproperties\n  S( " >> $5;
for i in $(seq 0 $((${END}-1))); do
    clause="    ("
    for j in $(seq 1 ${2}); do
        clause=$clause" broken_pipe$(($i+$j))>0 &"
    done
    clause=$clause" true ) |"
    echo "$clause" >> $5;
done
echo -e "    false )  // \"rate\"" >> $5
echo "endproperties" >> $5;

exit 0

