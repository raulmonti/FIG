#!/bin/bash

if [ $# -ne 6 ]
then
	echo "Must call with six parameter values"
	exit 1
else
	DISKNR=$1
	DISKRED=$2
	CTRLNR=$3
	CTRLRED=$4
	PROCNR=$5
	PROCRED=$6
fi

echo "/*"
echo " * Database with redundancy for the FIG tool"
echo " * Budde, Monti, D'Argenio | 2016"
echo " *"
echo " *{-"
echo " * Concept of the database computing system with redundancy:"
echo " * "
echo " * $PROCNR Types of Processors"
echo " * $CTRLNR Types of disk Controllers"
echo " * $DISKNR Disk clusters"
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
echo ""
echo "// -- Processors"
echo "const int PF = 2000;   // -- Processors' mean time to failure (in hours)"
echo "// -- unsupported! const double IPF = 0.01;  // -- Processors' inter-type failure rate"
echo "// -- Controllers"
echo "const int CF = 2000;  // -- Controllers' mean time to failure (in hours)"
echo "// -- Disk clusters"
echo "const int DF = 6000;  // -- Disks' mean time to failure (in hours)"


# DISKS

echo ""
echo ""
echo "///////////////////////////////////////////////////////////////////////"
echo "//"
echo "// -- Disk clusters | Total: $DISKNR"
echo "// --               | Redundancy: $DISKRED"
echo "// --               | Mean time to failure: DF"
echo "// --               | Num failures to breakdown per cluster: 2"

for (( i=1 ; i <= $DISKNR ; i++ )); do
    for (( j=1 ; j <= $DISKRED ; j++ )); do
        echo ""
        echo ""
        echo "module Disk${i}${j}"
        echo ""
        echo "	d${i}${j}f: bool init false;  // -- Disk failed?"
        echo "	d${i}${j}t: [1..2];           // -- Failure type"
        echo "	d${i}${j}clkF1: clock;        // -- Type 1 failure ~ exp(1/(2*DF))   "
        echo "	d${i}${j}clkF2: clock;        // -- Type 2 failure ~ exp(1/(2*DF))   "
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
        echo "	                                (d${i}${j}clkF1'= exponential(1/(2*DF))) &"
        echo "	                                (d${i}${j}clkF2'= exponential(1/(2*DF)));"
        echo "	[] d${i}${j}f & d${i}${j}t==2 @ d${i}${j}clkR2 -> (d${i}${j}f'= false) &"
        echo "	                                (d${i}${j}clkF1'= exponential(1/(2*DF))) &"
        echo "	                                (d${i}${j}clkF2'= exponential(1/(2*DF)));"
        echo "endmodule"
    done
done
echo ""


# CONTROLLERS

echo ""
echo ""
echo "///////////////////////////////////////////////////////////////////////"
echo "//"
echo "// -- Controllers | Total: $CTRLNR"
echo "// --             | Redundancy: $CTRLRED"
echo "// --             | Mean time to failure: CF"

for (( i=1 ; i <= $CTRLNR ; i++ )); do
    for (( j=1 ; j <= $CTRLRED ; j++ )); do
        echo ""
        echo ""
        echo "module Controller${i}${j}"
        echo ""
        echo "	c${i}${j}f: bool init false;  // -- Controller failed?"
        echo "	c${i}${j}t: [1..2];           // -- Failure type"
        echo "	c${i}${j}clkF1: clock;        // -- Type 1 failure ~ exp(1/(2*CF))"
        echo "	c${i}${j}clkF2: clock;        // -- Type 2 failure ~ exp(1/(2*CF))"
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
        echo "	                                (c${i}${j}clkF1'= exponential(1/(2*CF))) &"
        echo "	                                (c${i}${j}clkF2'= exponential(1/(2*CF)));"
        echo "	[] c${i}${j}f & c${i}${j}t==2 @ c${i}${j}clkR2 -> (c${i}${j}f'= false) &"
        echo "	                                (c${i}${j}clkF1'= exponential(1/(2*CF))) &"
        echo "	                                (c${i}${j}clkF2'= exponential(1/(2*CF)));"
        echo "endmodule"
    done
done
echo ""


# PROCESSORS

echo ""
echo ""
echo "///////////////////////////////////////////////////////////////////////"
echo "//"
echo "// -- Processors | Total: $PROCNR"
echo "// --            | Redundancy: $PROCRED"
echo "// --            | Mean time to failure: PF"

for (( i=1 ; i <= $PROCNR ; i++ )); do
	for (( j=1 ; j <= $PROCRED ; j++ )); do
        echo ""
        echo ""
        echo "module Processor${i}${j}"
        echo ""
        echo "	p${i}${j}f: bool init false;  // -- Processor failed?"
        echo "	p${i}${j}t: [1..2];           // -- Failure type"
        echo "	p${i}${j}clkF1: clock;        // -- Type 1 failure ~ exp(1/(2*PF))"
        echo "	p${i}${j}clkF2: clock;        // -- Type 2 failure ~ exp(1/(2*PF))"
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
        echo "	                                (p${i}${j}clkF1'= exponential(1/(2*PF))) &"
        echo "	                                (p${i}${j}clkF2'= exponential(1/(2*PF)));"
        echo "	[] p${i}${j}f & p${i}${j}t==2 @ p${i}${j}clkR2 -> (p${i}${j}f'= false) &"
        echo "	                                (p${i}${j}clkF1'= exponential(1/(2*PF))) &"
        echo "	                                (p${i}${j}clkF2'= exponential(1/(2*PF)));"
        echo "endmodule"
    done
done
echo ""


# PROPERTY

ECHO=`echo /bin/echo -en`
$ECHO "\n\n"
$ECHO "// -- Rate property to check\n"
$ECHO "S("
for (( i=1 ; i <= $DISKNR ; i++ )); do
	for (( j1=1 ; j1 <= $DISKRED ; j1++ )); do
	for (( j2=1 ; j2 <= $DISKRED ; j2++ )); do
		if (( ${j1} != ${j2} )); then
			$ECHO "\n   (d${i}${j1}f & d${i}${j2}f) |"
		fi
	done
	done
done
for (( i=1 ; i <= $CTRLNR ; i++ )); do
	$ECHO "\n   ("
	for (( j=1 ; j <= $CTRLRED ; j++ )); do
		$ECHO "c${i}${j}f & "
	done
	$ECHO "\b\b) |"
done
for (( i=1 ; i <= $PROCNR ; i++ )); do
	$ECHO "\n   ("
	for (( j=1 ; j <= $PROCRED ; j++ )); do
		$ECHO "p${i}${j}f & "
	done
	$ECHO "\b\b) |"
done
$ECHO "\b\b  \n ) // \"rate\"\n"

exit 0

