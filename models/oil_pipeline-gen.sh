#!/bin/bash

# Check calling arguments
if [ $# -ne 5 ]
then
	echo "[ERROR] Must call with five arguments"
	exit 1
else
	N=$1      # Number of nodes
	K=$2      # Number of consecutive fails needed
	T=$3      # Max simulation time
	DIST=$4   # Nodes lifetime distribution: 'E'xponential or 'W'eibull (Rayleigh)
	PARAM=$5  # Distribution parameter (lambda for 'E', sigma for 'W')
fi
if [[ ! "EW" =~ $DIST ]]
then
	echo "[ERROR] Lifetime distribution (i.e. the fourth calling argument)"
	echo "        must be either the capital letter 'E' or 'W'"
	exit 1
fi

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
echo " * or Rayleigh(s)* distributed and the repair-time will follow a lognormal(m,sd)"
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
echo ""
echo "// -- The following values were extracted from José Villén-Altamirano,"
echo "// -- \"RESTART simulation of non-Markov consecutive-k-out-of-n:F repairable"
echo "// -- systems\", Reliability Engineering and System Safety, Vol. 95, Issue 3,"
echo "// -- March 2010, pp. 247-254."
echo "// --"

if [[ "$DIST" == "E" ]]
then
	DIST="exponential($PARAM)"
elif [[ "$DIST" == "W" ]]
then
	DIST="rayleigh($PARAM)"
else
	echo "[ERROR] Lifetime distribution (i.e. the fourth calling argument)"
	echo "        must be either the capital letter 'E' or 'W'"
	exit 1
fi


echo "// -- const real s = "
echo ""
echo ""
echo ""
echo ""
echo ""
echo ""
echo ""
echo ""
echo ""
echo ""
