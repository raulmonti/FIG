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

for (( i=1 ; i<=$N ; i++ ))
do
	echo ""
	echo ""
	echo "module Node$i"
	echo "	N${i}clk: clock;          // -- Failure ~ $F_DIST"
	echo "	N${i}f: bool init false;  // -- Node failed?"
	echo "	[f$i!] !N${i}f @ N${i}clk -> (N${i}f'= true);"
	echo "	[r$i?]  N${i}f          -> (N${i}f'= false) & (N${i}clk'= $F_DIST);"
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
echo "	Rclk: clock;    // -- Repair ~ $R_DIST"
echo "	fix : [0..$N];  // -- Which node are we fixing now"
for i in $(seq $N); do echo "	N$i : bool init false;  // -- Node $i failed?"; done
echo ""
echo "	// -- Repair failed node right away"
for (( i=1 ; i<=$N ; i++ )); do
	echo "	[f$i?] fix == 0 -> (N$i'= true) & (fix' = $i) & (Rclk'= $R_DIST);"
done
echo ""
echo "	// -- Register failed node for later repairment"
for (( i=1 ; i<=$N ; i++ )); do
	echo "	[f$i?] fix > 0 -> (N$i'= true);"
done
echo ""
echo "	// -- Report repaired node and seek the next one"
for (( i=1 ; i<=$N ; i++ )); do
for (( k=1 ; k<=$N ; k++ )); do
	if [ $i -eq $k ]; then continue; fi
	echo "	[r$i!] fix == $i & N$k"
	for (( j=1 ; j<$k ; j++ )); do
		if [ $j -ne $i ]; then echo "	       & !N$j"; fi
	done
	echo "	       @ Rclk -> (N$i'= false) & (fix'= $k) & (Rclk'= $R_DIST);"
done; done
echo ""
echo "	// -- Report repairment of last failed node and go to sleep"
for (( i=1 ; i<=$N ; i++ )); do
	echo "	[r$i!] fix == $i"
	for (( j=1 ; j<=$N ; j++ )); do
		if [ $j -ne $i ]; then echo "	       & !N$j"; fi
	done
	echo "	       @ Rclk -> (N$i'= false) & (fix'= 0);"
done
echo "endmodule"
echo ""


# PROPERTIES

print() { /bin/echo -en "$1" 1>&2; }

print "S("
for (( i=1 ; i <= N-K+1 ; i++ )); do
	print "\n   ("
	for (( j=i ; j < i+K ; j++ )); do
		print "N$j & "
	done
	print "true) |"
done
print " false ) // \"rate\"\n"

