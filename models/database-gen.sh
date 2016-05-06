#!/bin/bash

if [ $# -ne 5 ]
then
	echo "[ERROR] Must call with five arguments:"
	echo "          1. system redundancy >= 2"
	echo "          2. number of Disk clusters"
	echo "          3. number of (disk) Controller types"
	echo "          4. number of Porcessor types"
	echo "          5. root mean fail time >= 2000"
	exit 1
else
	# Redundancy
	RED=$1
	D_RED=$((2+$RED))
	C_RED=$RED
	P_RED=$RED
	# Number of components
	D_NUM=$2
	C_NUM=$3
	P_NUM=$4
	# Mean fail time
	FTIME=$5
	D_FTIME=$((3*$FTIME))
	C_FTIME=$FTIME
	P_FTIME=$FTIME
fi

# Compute failure rate for given failure mean time and global REDundancy
compute_rate()
{
	if [ $# -eq 1 ]; then
		echo $1 $RED | awk '{ printf "%1.8f", 1/($1*$2) }';
	else
		echo "[ERROR] Need failure mean-time as single argument"; exit 1
	fi
}

echo "/*"
echo " * Database with redundancy for the FIG tool"
echo " * Budde, Monti, D'Argenio | 2016"
echo " *"
echo " *{-"
echo " * Concept of the database computing system with redundancy:"
echo " *"
echo " * $P_NUM Types of Processors"
echo " * $C_NUM Types of disk Controllers"
echo " * $D_NUM Disk clusters"
echo " *"
echo " * For redundancy 'RED'=2,3,..., there are RED components of each type"
echo " * of Processor and Controller, and RED+2 Disks on each disk cluster."
echo " * Processors, Controllers and Disks break down with different rates."
echo " * A breakdown in a processor of type i causes, with certain rate,"
echo " * a processor of the other type to break as well."
echo " * A single repairman chooses randomly among broken components, and"
echo " * fixes them up (one at a time) with one of two possible speed rates."
echo " *"
echo " * Initial state: all components in the system are operational."
echo " * Reference events: any system transition."
echo " * Rare event: system failure caused by RED simultaneously broken Processors"
echo " *             or Controllers of the same type, or RED broken Disks on the"
echo " *             same disk cluster."
echo " *-}"
echo " */"
echo ""
echo "// -- The following values were extracted from José Villén-Altamirano,"
echo "// -- \"Importance functions for RESTART simulation of highly-dependable"
echo "// -- systems\", Simulation, Vol. 83, Issue 12, December 2007, pp. 821-828."
echo "// --"
echo "// -- Processors"
P_FRATE=`compute_rate $P_FTIME`   # Processors fail rate: 1 / (NUM_FAIL_TYPES * P_FTIME)
echo "const int PF = ${P_FTIME};  // -- Processors' mean time to failure (in hours)"
echo "// -- unsupported! const double IPF = 0.01;  // -- Processors' inter-type failure rate"
echo "// --"
echo "// -- Controllers"
C_FRATE=`compute_rate $C_FTIME`   # Controllers fail rate: 1 / (NUM_FAIL_TYPES * C_FTIME)
echo "const int CF = ${C_FTIME};  // -- Controllers' mean time to failure (in hours)"
echo "// --"
echo "// -- Disk clusters"
D_FRATE=`compute_rate $D_FTIME`   # Disks fail rate: 1 / (NUM_FAIL_TYPES * D_FTIME)
echo "const int DF = ${D_FTIME};  // -- Disks' mean time to failure (in hours)"


# DISKS

echo ""
echo ""
echo "///////////////////////////////////////////////////////////////////////"
echo "//"
echo "// -- Disk clusters | Total: $D_NUM"
echo "// --               | Redundancy: $D_RED"
echo "// --               | Mean time to failure: DF"
echo "// --               | Num failures to breakdown per cluster: 2"

for (( i=1 ; i <= $D_NUM ; i++ )); do
    for (( j=1 ; j <= $D_RED ; j++ )); do
        echo ""
        echo ""
        echo "module Disk${i}${j}"
        echo ""
        echo "	d${i}${j}f: bool init false;  // -- Disk failed?"
        echo "	d${i}${j}t: [1..2];           // -- Failure type"
        echo "	d${i}${j}clkF1: clock;        // -- Type 1 failure ~ exp(1/(DF*$RED))"
        echo "	d${i}${j}clkF2: clock;        // -- Type 2 failure ~ exp(1/(DF*$RED))"
        echo "	d${i}${j}clkR1: clock;        // -- Repair for type 1 failure ~ exp(1.0)"
        echo "	d${i}${j}clkR2: clock;        // -- Repair for type 2 failure ~ exp(0.5)"
        echo ""
        echo "	[] !d${i}${j}f          @ d${i}${j}clkF1 -> (d${i}${j}f'= true)  &"
        echo "	                                (d${i}${j}t'= 1)     &"
        echo "	                                (d${i}${j}clkR1'= exponential(1.0));"
        echo "	[] !d${i}${j}f          @ d${i}${j}clkF2 -> (d${i}${j}f'= true)  &"
        echo "	                                (d${i}${j}t'= 2)     &"
        echo "	                                (d${i}${j}clkR2'= exponential(0.5));"
        echo "	[] d${i}${j}f & d${i}${j}t==1 @ d${i}${j}clkR1 -> (d${i}${j}f'= false) &"
        echo "	                                (d${i}${j}clkF1'= exponential(${D_FRATE})) &"
        echo "	                                (d${i}${j}clkF2'= exponential(${D_FRATE}));"
        echo "	[] d${i}${j}f & d${i}${j}t==2 @ d${i}${j}clkR2 -> (d${i}${j}f'= false) &"
        echo "	                                (d${i}${j}clkF1'= exponential(${D_FRATE})) &"
        echo "	                                (d${i}${j}clkF2'= exponential(${D_FRATE}));"
        echo "endmodule"
    done
done
echo ""


# CONTROLLERS

echo ""
echo ""
echo "///////////////////////////////////////////////////////////////////////"
echo "//"
echo "// -- Controllers | Total: $C_NUM"
echo "// --             | Redundancy: $C_RED"
echo "// --             | Mean time to failure: CF"

for (( i=1 ; i <= $C_NUM ; i++ )); do
    for (( j=1 ; j <= $C_RED ; j++ )); do
        echo ""
        echo ""
        echo "module Controller${i}${j}"
        echo ""
        echo "	c${i}${j}f: bool init false;  // -- Controller failed?"
        echo "	c${i}${j}t: [1..2];           // -- Failure type"
        echo "	c${i}${j}clkF1: clock;        // -- Type 1 failure ~ exp(1/(CF*$RED))"
        echo "	c${i}${j}clkF2: clock;        // -- Type 2 failure ~ exp(1/(CF*$RED))"
        echo "	c${i}${j}clkR1: clock;        // -- Repair for type 1 failure ~ exp(1.0)"
        echo "	c${i}${j}clkR2: clock;        // -- Repair for type 2 failure ~ exp(0.5)"
        echo ""
        echo "	[] !c${i}${j}f          @ c${i}${j}clkF1 -> (c${i}${j}f'= true)  &"
        echo "	                                (c${i}${j}t'= 1)     &"
        echo "	                                (c${i}${j}clkR1'= exponential(1.0));"
        echo "	[] !c${i}${j}f          @ c${i}${j}clkF2 -> (c${i}${j}f'= true)  &"
        echo "	                                (c${i}${j}t'= 2)     &"
        echo "	                                (c${i}${j}clkR2'= exponential(0.5));"
        echo "	[] c${i}${j}f & c${i}${j}t==1 @ c${i}${j}clkR1 -> (c${i}${j}f'= false) &"
        echo "	                                (c${i}${j}clkF1'= exponential(${C_FRATE})) &"
        echo "	                                (c${i}${j}clkF2'= exponential(${C_FRATE}));"
        echo "	[] c${i}${j}f & c${i}${j}t==2 @ c${i}${j}clkR2 -> (c${i}${j}f'= false) &"
        echo "	                                (c${i}${j}clkF1'= exponential(${C_FRATE})) &"
        echo "	                                (c${i}${j}clkF2'= exponential(${C_FRATE}));"
        echo "endmodule"
    done
done
echo ""


# PROCESSORS

echo ""
echo ""
echo "///////////////////////////////////////////////////////////////////////"
echo "//"
echo "// -- Processors | Total: $P_NUM"
echo "// --            | Redundancy: $P_RED"
echo "// --            | Mean time to failure: PF"

for (( i=1 ; i <= $P_NUM ; i++ )); do
	for (( j=1 ; j <= $P_RED ; j++ )); do
        echo ""
        echo ""
        echo "module Processor${i}${j}"
        echo ""
        echo "	p${i}${j}f: bool init false;  // -- Processor failed?"
        echo "	p${i}${j}t: [1..2];           // -- Failure type"
        echo "	p${i}${j}clkF1: clock;        // -- Type 1 failure ~ exp(1/(PF*$RED))"
        echo "	p${i}${j}clkF2: clock;        // -- Type 2 failure ~ exp(1/(PF*$RED))"
        echo "	p${i}${j}clkR1: clock;        // -- Repair for type 1 failure ~ exp(1.0)"
        echo "	p${i}${j}clkR2: clock;        // -- Repair for type 2 failure ~ exp(0.5)"
        echo ""
        echo "	[] !p${i}${j}f          @ p${i}${j}clkF1 -> (p${i}${j}f'= true)  &"
        echo "	                                (p${i}${j}t'= 1)     &"
        echo "	                                (p${i}${j}clkR1'= exponential(1.0));"
        echo "	[] !p${i}${j}f          @ p${i}${j}clkF2 -> (p${i}${j}f'= true)  &"
        echo "	                                (p${i}${j}t'= 2)     &"
        echo "	                                (p${i}${j}clkR2'= exponential(0.5));"
        echo "	[] p${i}${j}f & p${i}${j}t==1 @ p${i}${j}clkR1 -> (p${i}${j}f'= false) &"
        echo "	                                (p${i}${j}clkF1'= exponential(${P_FRATE})) &"
        echo "	                                (p${i}${j}clkF2'= exponential(${P_FRATE}));"
        echo "	[] p${i}${j}f & p${i}${j}t==2 @ p${i}${j}clkR2 -> (p${i}${j}f'= false) &"
        echo "	                                (p${i}${j}clkF1'= exponential(${P_FRATE})) &"
        echo "	                                (p${i}${j}clkF2'= exponential(${P_FRATE}));"
        echo "endmodule"
    done
done
echo ""


# PROPERTY

# We need to build the M-combinations of N...
fac() { if [ $1 -lt 2 ]; then echo $1; else echo $(($1*`fac $(($1 - 1))`)); fi }
comb() { echo "scale=0; `fac $1` / (`fac $2` * `fac $(($1-$2))`)" | bc -l; }
print() { /bin/echo -en "$@" 1>&2; }

print "S("

# ...for Disks...
M=$RED
N=$D_RED
declare -a INDEX=($(seq $((N-M+1)) $N))  # highest M-combinations indices
for (( d=1 ; d <= $D_NUM ; d++ )); do
	declare -a index=($(seq $M))  # lowest  M-combinations indices
	i=$((M-1))
	while [ true ]; do
		# print this combination
		print "\n   ("
		for idx in ${index[@]}; do
			print "d${d}${idx}f & "
		done
		print "true) |"
		# and compute next one
		reset=false
		while (( $i >= 0 )) && (( ${index[i]} >= ${INDEX[i]} )); do
			i=$((i-1))
			reset=true
		done
		if (( $i < 0 )); then
			break  # all combinations were covered
		else
			index[$i]=$((${index[i]} + 1))
		fi
		if [ $reset ]; then 
			for (( j=i+1 ; j<M ; j++ )); do
				index[$j]=$((${index[$((j-1))]}+1))
			done
			i=$((M-1))
		fi
	done
done

# ...for Controllers...
M=$RED
N=$C_RED
declare -a INDEX=($(seq $((N-M+1)) $N))  # highest M-combinations indices
for (( c=1 ; c <= $C_NUM ; c++ )); do
	declare -a index=($(seq $M))  # lowest  M-combinations indices
	i=$((M-1))
	while [ true ]; do
		# print this combination
		print "\n   ("
		for idx in ${index[@]}; do
			print "c${c}${idx}f & "
		done
		print "true) |"
		# and compute next one
		reset=false
		while (( $i >= 0 )) && (( ${index[i]} >= ${INDEX[i]} )); do
			i=$((i-1))
			reset=true
		done
		if (( $i < 0 )); then
			break  # all combinations were covered
		else
			index[$i]=$((${index[i]} + 1))
		fi
		if [ $reset ]; then 
			for (( j=i+1 ; j<M ; j++ )); do
				index[$j]=$((${index[$((j-1))]}+1))
			done
			i=$((M-1))
		fi
	done
done

# ...and for Processors...
M=$RED
N=$P_RED
declare -a INDEX=($(seq $((N-M+1)) $N))  # highest M-combinations indices
for (( p=1 ; p <= $P_NUM ; p++ )); do
	declare -a index=($(seq $M))  # lowest  M-combinations indices
	i=$((M-1))
	while [ true ]; do
		# print this combination
		print "\n   ("
		for idx in ${index[@]}; do
			print "p${p}${idx}f & "
		done
		print "true) |"
		# and compute next one
		reset=false
		while (( $i >= 0 )) && (( ${index[i]} >= ${INDEX[i]} )); do
			i=$((i-1))
			reset=true
		done
		if (( $i < 0 )); then
			break  # all combinations were covered
		else
			index[$i]=$((${index[i]} + 1))
		fi
		if [ $reset ]; then 
			for (( j=i+1 ; j<M ; j++ )); do
				index[$j]=$((${index[$((j-1))]}+1))
			done
			i=$((M-1))
		fi
	done
done

print " false ) // \"rate\"\n"

exit 0

