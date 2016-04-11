#!/bin/sh

DISKNR=$1
DISKRED=$2
CTRLNR=$3
CTRLRED=$4
PROCNR=$5
PROCRED=$6


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

i=1
while [ $i -le $DISKNR ]; do
    j=1
    while [ $j -le $DISKRED ]; do
	echo ""
	echo ""
        echo "module Disk$i$j"
        echo "	fdsk$i$j: [1..2];"
        echo "	stdsk$i$j:   [0..1]; // -- 0 = up, 1 = down"
        echo "	ckF1dsk$i$j:   clock;  // -- Failure ~ exp(1/(2*DF))     Disk failing to type1"
        echo "	ckF2dsk$i$j:   clock;  // -- Failure ~ exp(1/(2*DF))     Disk failing to type2"
        echo "	ckR1dsk$i$j:   clock;  // -- Repair for type 1 failures ~ exp(1.0)"
        echo "	ckR2dsk$i$j:   clock;  // -- Repair for type 2 failures ~ exp(0.5)"
        echo ""
        echo "	[] stdsk$i$j==0 @ ckF1dsk$i$j -> (fdsk$i$j'= 1) &"
        echo "				    (stdsk$i$j'=1) &"
        echo "				    (ckR1dsk$i$j'= exponential(1.0));"
        echo "	[] stdsk$i$j==0 @ ckF2dsk$i$j -> (fdsk$i$j'= 2) &"
        echo "				    (stdsk$i$j'=1) &"
        echo "				    (ckR2dsk$i$j'= exponential(0.5));"
        echo "	[] stdsk$i$j==1 & fdsk$i$j==1 @ ckR1dsk$i$j -> (stdsk$i$j'=0) &"
        echo "						     (ckF1dsk$i$j'= exponential(1/(2*DF))) &"
        echo "						     (ckF2dsk$i$j'= exponential(1/(2*DF)));"
        echo "	[] stdsk$i$j==1 & fdsk$i$j==2 @ ckR2dsk$i$j -> (stdsk$i$j'=0) &"
        echo "						     (ckF1dsk$i$j'= exponential(1/(2*DF))) &"
        echo "						     (ckF2dsk$i$j'= exponential(1/(2*DF)));"
	echo "endmodule"
	let j++
    done
    let i++
done



# CONTROLERS

echo "///////////////////////////////////////////////////////////////////////"
echo "//"
echo "// -- Controllers   | Total: $CTRLNR"
echo "// --               | Redundancy: $CTRLRED"
echo "// --               | Mean time to failure: CF"

i=1
while [ $i -le $CTRLNR ]; do
    j=1
    while [ $j -le $CTRLRED ]; do
	echo ""
	echo ""
        echo "	module Controller$i$j"
        echo "	fctrl$i$j: [1..2];"
        echo "	stctrl$i$j:   [0..1]; // -- 0 = up, 1 = down"
        echo "	ckF1ctrl$i$j:   clock;  // -- Failure ~ exp(1/(2*CF))     Disk failing to type1"
        echo "	ckF2ctrl$i$j:   clock;  // -- Failure ~ exp(1/(2*CF))     Disk failing to type2"
        echo "	ckR1ctrl$i$j:   clock;  // -- Repair for type 1 failures ~ exp(1.0)"
        echo "	ckR2ctrl$i$j:   clock;  // -- Repair for type 2 failures ~ exp(0.5)"
        echo ""
        echo "	[] stctrl$i$j==0 @ ckF1ctrl$i$j -> (fctrl$i$j'= 1) &"
        echo "				    (stctrl$i$j'=1) &"
        echo "				    (ckR1ctrl$i$j'= exponential(1.0));"
        echo "	[] stctrl$i$j==0 @ ckF2ctrl$i$j -> (fctrl$i$j'= 2) &"
        echo "				    (stctrl$i$j'=1) &"
        echo "				    (ckR2ctrl$i$j'= exponential(0.5));"
        echo "	[] stctrl$i$j==1 & fctrl$i$j==1 @ ckR1ctrl$i$j -> (stctrl$i$j'=0) &"
        echo "						     (ckF1ctrl$i$j'= exponential(1/(2*CF))) &"
        echo "						     (ckF2ctrl$i$j'= exponential(1/(2*CF)));"
        echo "	[] stctrl$i$j==1 & fctrl$i$j==2 @ ckR2ctrl$i$j -> (stctrl$i$j'=0) &"
        echo "						     (ckF1ctrl$i$j'= exponential(1/(2*CF))) &"
        echo "						     (ckF2ctrl$i$j'= exponential(1/(2*CF)));"
	echo "endmodule"
	let j++
    done
    let i++
done


# Processors

echo "///////////////////////////////////////////////////////////////////////"
echo "//"
echo "// -- Processors    | Total: $PROCNR"
echo "// --               | Redundancy: $PROCRED"
echo "// --               | Mean time to failure: PF"

i=1
while [ $i -le $PROCNR ]; do
    j=1
    while [ $j -le $PROCRED ]; do
	echo ""
	echo ""
        echo "	module Processor$i$j"
        echo "	fproc$i$j: [1..2];"
        echo "	stproc$i$j:   [0..1]; // -- 0 = up, 1 = down"
        echo "	ckF1proc$i$j:   clock;  // -- Failure ~ exp(1/(2*PF))     Disk failing to type1"
        echo "	ckF2proc$i$j:   clock;  // -- Failure ~ exp(1/(2*PF))     Disk failing to type2"
        echo "	ckR1proc$i$j:   clock;  // -- Repair for type 1 failures ~ exp(1.0)"
        echo "	ckR2proc$i$j:   clock;  // -- Repair for type 2 failures ~ exp(0.5)"
        echo ""
        echo "	[] stproc$i$j==0 @ ckF1proc$i$j -> (fproc$i$j'= 1) &"
        echo "				    (stproc$i$j'=1) &"
        echo "				    (ckR1proc$i$j'= exponential(1.0));"
        echo "	[] stproc$i$j==0 @ ckF2proc$i$j -> (fproc$i$j'= 2) &"
        echo "				    (stproc$i$j'=1) &"
        echo "				    (ckR2proc$i$j'= exponential(0.5));"
        echo "	[] stproc$i$j==1 & fproc$i$j==1 @ ckR1proc$i$j -> (stproc$i$j'=0) &"
        echo "						     (ckF1proc$i$j'= exponential(1/(2*PF))) &"
        echo "						     (ckF2proc$i$j'= exponential(1/(2*PF)));"
        echo "	[] stproc$i$j==1 & fproc$i$j==2 @ ckR2proc$i$j -> (stproc$i$j'=0) &"
        echo "						     (ckF1proc$i$j'= exponential(1/(2*PF))) &"
        echo "						     (ckF2proc$i$j'= exponential(1/(2*PF)));"
	echo "endmodule"
	let j++
    done
    let i++
done
