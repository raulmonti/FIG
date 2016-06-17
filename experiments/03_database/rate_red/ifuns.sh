#!/bin/bash
#
# Author:  Carlos E. Budde
# Date:    17.06.2016
# License: GPLv3
#
#   Ad hoc / Composition importance function constructors
#   for the database experiment from the FIG projet.
#
# NOTE: this script should be sourced from another script
# Cool site: http://wiki.bash-hackers.org
#


# Print V-A's ad hoc importance function "cl-oc(t)",
# i.e. "min cut set cardinality minus the number of operational components
#       in the cut set with the lowest number of operational components"
min_num_oc() {
	if [ $# -ne 4 ]; then
		echo "Requires redundancy value and # of {disks clusters,"
		echo "controller types, processor types}, in that order"
		return 1
	fi
	local RED=$1
	local ND=$2
	local NC=$3
	local NP=$4
	local ECHO=`echo "echo -en"`
	# Notice "cl-oc(t)" equals the number of broken components
	# in the cut set with the highest number of broken components. 
	for (( i=1 ; i<=ND ; i++ )); do
		$ECHO "max("
		for (( j=1 ; j<=RED+2 ; j++)); do $ECHO "d${i}${j}f+"; done
		$ECHO "0,"
	done
	for (( i=1 ; i<=NC ; i++ )); do
		$ECHO "max("
		for (( j=1 ; j<=RED ; j++)); do $ECHO "c${i}${j}f+"; done
		$ECHO "0,"
	done
	for (( i=1 ; i<=NP ; i++ )); do
		$ECHO "max("
		for (( j=1 ; j<=RED ; j++)); do $ECHO "p${i}${j}f+"; done
		$ECHO "0,"
	done
	$ECHO "$RED"
	for (( i=0 ; i<ND+NC+NP ; i++ )); do $ECHO ")"; done
	$ECHO ";0;$RED"
	return 0
}


# For given redundancy, print a composition function for --acomp
# which adds the product of all units from each component type,
# viz. all-disks product + all-controllers product + all-processors product
comp_fun_coarse() {
	if [ $# -ne 4 ]; then
		echo "Requires redundancy value and # of {disks clusters,"
		echo "controller types, processor types}, in that order"
		return 1
	fi
	local RED=$1
	local ND=$2
	local NC=$3
	local NP=$4
	local MINVAL=3
	local MAXVAL=`bc <<< "2^($((ND*(RED+2))))+2^$((NC*RED))+2^$((NP*RED))"`
	local ECHO=`echo "echo -en"`
	$ECHO "("
	for (( i=1 ; i<=ND ; i++ )); do
		for (( j=1 ; j<=RED+2 ; j++ )); do $ECHO "Disk${i}${j}*"; done
	done
	$ECHO "1)+("
	for (( i=1 ; i<=NC ; i++ )); do
		for (( j=1 ; j<=RED ; j++ )); do $ECHO "Controller${i}${j}*"; done
	done
	$ECHO "1)+("
	for (( i=1 ; i<=NP ; i++ )); do
		for (( j=1 ; j<=RED ; j++ )); do $ECHO "Processor${i}${j}*"; done
	done
	$ECHO "1);${MINVAL};${MAXVAL};1"
	return 0
}


# For given redundancy, print a composition function for --acomp
# which adds the products of the units from each component instance,
# e.g. cluster1-disks product + cluster2-disks product + ...
#      ... + controllers1 product + controllers2 product + ...
comp_fun_med() {
	if [ $# -ne 4 ]; then
		echo "Requires redundancy value and # of {disks clusters,"
		echo "controller types, processor types}, in that order"
		return 1
	fi
	local RED=$1
	local ND=$2
	local NC=$3
	local NP=$4
	local MINVAL=$((ND+NC+NP))
	local MAXVAL=`bc <<< "$ND*2^$((RED+2)) + $NC*2^$RED + $NP*2^$RED"`
	local ECHO=`echo "echo -en"`
	for (( i=1 ; i<=ND ; i++ )); do
		$ECHO "("
		for (( j=1 ; j<=RED+2 ; j++ )); do $ECHO "Disk${i}${j}*"; done
		$ECHO "1)+"
	done
	$ECHO "0+"
	for (( i=1 ; i<=NC ; i++ )); do
		$ECHO "("
		for (( j=1 ; j<=RED ; j++ )); do $ECHO "Controller${i}${j}*"; done
		$ECHO "1)+"
	done
	$ECHO "0+"
	for (( i=1 ; i<=NP ; i++ )); do
		$ECHO "("
		for (( j=1 ; j<=RED ; j++ )); do $ECHO "Processor${i}${j}*"; done
		$ECHO "1)+"
	done
	$ECHO "0;${MINVAL};${MAXVAL};1"
	return 0
}


# For given redundancy, print a composition function for --acomp
# which adds the products of each possible failure configuration,
# e.g. a natural mapping of the rare event property
comp_fun_fine() {
	if [ $# -ne 4 ]; then
		echo "Requires redundancy value and # of {disks clusters,"
		echo "controller types, processor types}, in that order"
		return 1;
	fi
	local RED=$1
	local ND=$2
	local NC=$3
	local NP=$4
	local MINVAL=$((ND+NC+NP))
	local MAXVAL=`bc <<< "$ND*2^$((RED+2)) + $NC*2^$RED + $NP*2^$RED"`
	local ECHO=`echo "echo -en"`

	$ECHO 'TODO!'; return 1
	# We need to compute the combination indices :(
	# Copy code for property generation from database-gen.sh

	return 0
}


return 0

