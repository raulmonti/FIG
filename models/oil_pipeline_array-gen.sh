#!/bin/bash

# Check calling arguments
if [ $# -ne 5 ]
then
	echo "[ERROR] Must call with five arguments:"
	echo "          1. number of system nodes      ('n')"
	echo "          2. number of consecutive fails ('k')"
	echo "          3. max simulation time"
	echo "          4. nodes lifetime distribution: 'E' or 'W'"
	echo "          5. distribution parameter"
	exit 1
else
	N=$1  # Number of nodes
	K=$2  # Number of consecutive fails needed
	T=$3  # Max simulation time
	D=$4  # Nodes lifetime distrib: 'E'xponential or 'W'eibull (Rayleigh)
	P=$5  # Distribution parameter (lambda for 'E', sigma for 'W')
fi
if [ $N -lt 5 ]
then
	echo "[ERROR] The number of nodes (i.e. the first calling argument)"
	echo "        must be at least five."
	exit 1
elif [ $K -gt $N ]
then
	echo "[ERROR] The number of consecutive fails (i.e. the second calling argument)"
	echo "        must be smaller than the number of nodes."
	exit 1
elif [[ ! "EW" =~ "$D" ]]
then
	echo "[ERROR] The nodes lifetime distribution (i.e. the fourth calling argument)"
	echo "        must be either the capital letter 'E' or 'W'"
	exit 1
fi


# INTRO

echo "/*"
echo " * Non-Markov consecutive-k-out-of-n:F repairable system for the FIG tool"
echo " * Budde, Monti, D'Argenio | 2016"
echo " *"
echo " *{-"
echo " * Concept of the Non-Markov consecutive-k-out-of-n:F repairable system:"
echo " *"
echo " * ---> (P1) ---> (P2) ---> ··· ---> (Pn) --->"
echo " *"
echo " * For a graphical illustration imagine an oil pipeline with 'n' equally-spaced"
echo " * pressure pumps along it, {P1..Pn}. Each pump sends oil to the next and it can"
echo " * only fail and get repaired (viz. there are just two possible states {up,down})"
echo " * The whole pipeline \"fails\" as soon as 'k' (or more) consecutive pumps fail."
echo " * A single repairman attends one broken pump at a time and fixes it."
echo " *"
echo " * In this implementation the pumps lifetime will be either exponentially(l)"
echo " * or Rayleigh(s)* distributed and the repair-time will follow a Log-normal(m,sd)"
echo " * distribution. The probability of failure of each pump will be independent"
echo " * of the system's general state; the (k-1)-step Markov dependence of some"
echo " * typical implementations is thus dropped. Also the repairman will apply a"
echo " * static repair policy: it will always choose the first failed pump, that is"
echo " * the broken pump with smallest index in {P1..Pn}."
echo " *"
echo " * Initial state: all pumps are up."
echo " * Reference event: any system transition."
echo " * Rare event: system failure caused by at least 'k' consecutive failed pumps"
echo " *"
echo " * [*] viz. Weibull with shape == 2 and rate == s*sqrt(2)"
echo " *-}"
echo " */"


# PARAMETERS

echo ""
echo "// -- The following values were extracted from José Villén-Altamirano,"
echo "// -- \"RESTART simulation of non-Markov consecutive-k-out-of-n:F repairable"
echo "// -- systems\", Reliability Engineering and System Safety, Vol. 95, Issue 3,"
echo "// -- March 2010, pp. 247-254."
echo "// --"
if [[ "$D" == "E" ]]; then
	F_DIST="exponential($P)"
elif [[ "$D" == "W" ]]; then
	F_DIST="rayleigh($P)"
else
	echo "[ERROR] Bad lifetime distribution"; exit 1;
fi
LN_M=1.21
LN_SD=0.8
R_DIST="lognormal($LN_M,$LN_SD)"
echo "// -- Nodes lifetime ~ $F_DIST"
echo "// -- Repair time ~ Log-normal($LN_M,$LN_SD)"
echo ""


# NODES (e.g. "oil pressure-pumps")

echo ""
echo ""
echo "///////////////////////////////////////////////////////////////////////"
echo "//"
echo "// -- Nodes | Total: $N"
echo "// --       | Consecutive fails needed: $K"
echo "// --       | Lifetime distribution: $F_DIST"


LAST=$[$N - 1]

echo "// -- Nodes are numbered from 0 to $LAST"

for (( i=0 ; i<=$LAST ; i++ ))
do
	echo ""
	echo ""
	echo "module Node$i"
	echo "	N${i}clk: clock;          // -- Failure ~ $F_DIST"
	echo "	N${i}: bool init false;   // -- Node failed?"
	echo "	[f$i!] !N$i @ N${i}clk -> (N$i'= true);"
	echo "	[r$i?]  N$i          -> (N$i'= false) & (N${i}clk'= $F_DIST);"
	echo "endmodule"
done
echo ""


# REPAIRMAN

echo ""
echo ""
echo "///////////////////////////////////////////////////////////////////////"
echo "//"
echo "// -- Repairman | Fixes first broken node found"
echo "// --           | Repair-time distribution: Log-normal($LN_M,$LN_SD)"
echo ""
echo ""
echo "module Repairman"
echo ""
echo "Rclk: clock;    // -- Repair ~ $R_DIST"
echo "fix : [-1 .. $LAST] init -1;  // -- Which node are we fixing now (-1 if we are not fixing any of them)"
echo "fails [$N] : bool init false; // -- fails[j] == true iff node Node j has failed"
echo ""
echo "// -- Repair failed node right away"
for (( i=0 ; i<=$LAST ; i++ )); do
	echo "[f$i?] fix == -1 -> (fails[${i}]'= true) & (fix' = $i) & (Rclk'= $R_DIST);"
done
echo ""
echo "// -- Register failed node for later repairment"
for (( i=0 ; i<=$LAST ; i++ )); do
	echo "[f$i?] fix >= 0 -> (fails[${i}]'= true);"
done
echo ""
echo "// -- Report repaired node and seek the next one, if any"
echo "// -- fstexclude(fails, j) returns the first position 0 <= i < fails.size of \"fails\" such that fails[i]==true and i != j, "
echo "// -- or -1 if no such i exists"
echo ""
for (( i=0 ; i<=$LAST ; i++ )); do
   echo "[r$i!] fix == ${i} @ Rclk -> (fails[${i}]' = false) & (fix' = fstexclude(fails, ${i})) & (Rclk' = $R_DIST);"
done
echo "endmodule"
echo ""

# PROPERTIES

echo "properties"
echo "S ( consec(fails, ${K}) ) // \"rate\""
echo "endproperties"

