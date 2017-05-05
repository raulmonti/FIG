#!/bin/bash
#
# Author:  Carlos E. Budde
# Date:    02.04.2017
# License: GPLv3
#
#   Ad hoc & composition importance function constructors
#   for the oil_pipeline experiment from the FIG projet.
#
# NOTE: this script should be sourced from another script
# Cool site: http://wiki.bash-hackers.org
#


# Max number of continuously broken nodes (aka oil pumps)
# Intended for ad hoc ifun
max_continuous_failures_adhoc() {
	if [ $# -ne 2 ]; then
		echo "[ERROR] Requires # of nodes and # of consecutive failures"
		echo "        that make up a system failure (aka 'N' and 'K')"
		return 1
	fi
	local N=$1
	local K=$2
	local ECHO=`echo "echo -en"`
	EXPR="max("
	for (( i=1 ; i<=N-K+1 ; i++ )) do
		CLUSTER=`printf "(broken_pipe%d>0)+" $(seq $i $((i+K-1)))`
		EXPR+=$CLUSTER"0,"
	done
	EXPR+="0);0;$K"
	echo $EXPR
}

# Max number of continuously broken nodes (aka oil pumps)
# Intended for acomp ifun
max_continuous_failures_acomp() {
	if [ $# -ne 2 ]; then
		echo "[ERROR] Requires # of nodes and # of consecutive failures"
		echo "        that make up a system failure (aka 'N' and 'K')"
		return 1
	fi
	local N=$1
	local K=$2
	local ECHO=`echo "echo -en"`
	EXPR="max("
	for (( i=1 ; i<=N-K+1 ; i++ )) do
		CLUSTER=`printf "BE_pipe%d+" $(seq $i $((i+K-1)))`
		EXPR+=$CLUSTER"0,"
	done
	EXPR+="0);0;$K"
	echo $EXPR
}

# Add the product of the number of continuously broken nodes
# Inteded for acomp ifun
sum_continuous_failures_acomp() {
	if [ $# -ne 2 ]; then
		echo "[ERROR] Requires # of nodes and # of consecutive failures"
		echo "        that make up a system failure (aka 'N' and 'K')"
		return 1
	fi
	local N=$1
	local K=$2
	local ECHO=`echo "echo -en"`
	EXPR=""
	for (( i=1 ; i<=N-K+1 ; i++ )) do
		CLUSTER=`printf "BE_pipe%d*" $(seq $i $((i+K-1)))`
		EXPR+=$CLUSTER"1+"
	done
	EXPR+="0;$((N-K+1));$(((N-K+1)*(2**K)))"
	echo $EXPR
}

# Helper function for the optimised functions below
# Inteded for acomp ifun
sum_continuous_failures_acomp_remainder() {
	if [ $# -ne 2 ]; then
		echo "[ERROR] Requires # of nodes and # of consecutive failures"
		echo "        that make up a system failure (aka 'N' and 'K')"
		return 1
	fi
	local N=$1
	local K=$2
	local ECHO=`echo "echo -en"`
	local VARi="BE_pipe%d"
	#local VARi="x%d"
	local REMAINDER=""
	for (( i=(N-K+1)-(N-K+1)%K+1 ; i<=N-K+1 ; i++ )) do
		CLUSTER=`printf "$VARi*" $(seq $i $((i+K-1)))`
		REMAINDER+=$CLUSTER"1+"
	done
	REMAINDER+="0;$((N-K+1));$(((N-K+1)*(2**K)))"
	echo $REMAINDER
}

# Add the product of the number of continuously broken nodes
# Optimized for K=3: math expression uses less operations
# Inteded for acomp ifun
sum_continuous_failures_acomp_K3() {
	if [ $# -ne 1 ]; then
		echo "[ERROR] Requires # of nodes in the system (aka 'N')"
		return 1
	fi
	local N=$1
	local K=3
	local ECHO=`echo "echo -en"`
	local VARi="BE_pipe%d"
	#local VARi="x%d"
	EXPR=""
	for (( i=1 ; i<=(N-K+1)/K ; i++ )) do
		EXPR+=`printf "$VARi*($VARi*$VARi+$VARi*($VARi+$VARi))+" \
		       $((3*i+0)) $((3*i-2)) $((3*i-1))                  \
			   $((3*i+1)) $((3*i-1)) $((3*i+2))`
	done
	EXPR+="`sum_continuous_failures_acomp_remainder $N $K`"
	echo $EXPR
}
