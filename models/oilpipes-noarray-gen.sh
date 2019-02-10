#!/bin/env bash

# CALLING ARGUMENTS
if [ $# -ne 5 ]; then
	echo "[ERROR] Invalid invocation, call with 5 arguments:";
	echo "  1. Number of nodes (\"pressure pumps\")    (aka N, e.g. 20)";
	echo "  2. Number of consecutive broken nodes    (aka K, e.g. 3 )";
	echo "  3. Nodes  fail  distribution (e.g. 'rayleigh(798.0)'    )";
	echo "  4. Nodes repair distribution (e.g. 'lognormal(1.21,0.8)')";
	echo "  5. Output filename           (e.g. \"oilpipe_20_3_ray.sa\")";
	exit 1;
else
	N=$1;
	K=$2;
	F_DIST=$3;
	R_DIST=$4;
	MODEL=$5;
fi

# INTRO
echo "/*" > $MODEL;  # XXX NOTE: clears model file, if any
echo " * Non-Markov consecutive-K-out-of-N:F repairable system for the FIG tool" >> $MODEL;
echo " * Budde, Monti, D'Argenio | 2017" >> $MODEL;
echo " *" >> $MODEL;
echo " *{-" >> $MODEL;
echo " * Concept of the Non-Markov consecutive-k-out-of-n:F repairable system:" >> $MODEL;
echo " *" >> $MODEL;
echo " * ---> (P1) ---> (P2) ---> ··· ---> (Pn) --->" >> $MODEL;
echo " *" >> $MODEL;
echo " * For a graphical illustration imagine an oil pipeline with 'n' equally-spaced" >> $MODEL;
echo " * pressure pumps along it, {P1..Pn}. Each pump sends oil to the next and it can" >> $MODEL;
echo " * only fail and get repaired (viz. there are just two possible states {up,down})" >> $MODEL;
echo " * The whole pipeline \"fails\" as soon as 'k' (or more) consecutive pumps fail." >> $MODEL;
echo " * A single repairman attends one broken pump at a time and fixes it." >> $MODEL;
echo " *" >> $MODEL;
echo " * In this implementation the probability of failure of each pump will be " >> $MODEL;
echo " * independent of the system's general state; the (k-1)-step Markov dependence " >> $MODEL;
echo " * that some authors assume is thus dropped. Also the repairman will apply a" >> $MODEL;
echo " * static repair policy: it will always choose the first failed pump, that is," >> $MODEL;
echo " * the broken pump with smallest index in {P1..Pn}." >> $MODEL;
echo " *" >> $MODEL;
echo " * Initial state: all pumps are up." >> $MODEL;
echo " * Reference event: any system transition." >> $MODEL;
echo " * Rare event: system failure caused by at least 'k' consecutive failed pumps" >> $MODEL;
echo " *-}" >> $MODEL;
echo " */" >> $MODEL;

# PARAMETERS IN THE LITERATURE
echo "" >> $MODEL;
echo "// The following distributions are used in page 252, Section 4.1," >> $MODEL;
echo "// \"System steady-state unavailability and MTBF\" of José Villén-Altamirano" >> $MODEL;
echo "// \"RESTART simulation of non-Markov consecutive-k-out-of-n:F repairable" >> $MODEL;
echo "// systems\", Reliability Engineering and System Safety, Vol. 95, Issue 3," >> $MODEL;
echo "// March 2010, pp. 247-254:" >> $MODEL;
echo "//  · Repair time ~ Log-normal(1.21,0.8)" >> $MODEL;
echo "//  · Nodes lifetime ~ Exponential(lambda) or Rayleigh(sigma)" >> $MODEL;
echo "//                     for (lambda,sigma) in {(0.001 ,  798.0  )," >> $MODEL;
echo "//                                            (0.0003, 2659.615)," >> $MODEL;
echo "//                                            (0.0001, 7978.845)}" >> $MODEL;
echo "// NOTE: Rayleigh(s) distribution is actually" >> $MODEL;
echo "//       a Weibull distribution with shape == 2 and rate == s*sqrt(2)" >> $MODEL;
echo -en "\n\n" >> $MODEL;

# NODES (e.g. "oil pressure-pumps")
echo "" >> $MODEL;
echo "" >> $MODEL;
echo "///////////////////////////////////////////////////////////////////////" >> $MODEL;
echo "//" >> $MODEL;
echo "// Nodes | Total: $N" >> $MODEL;
echo "//       | Consecutive fails needed: $K" >> $MODEL;
echo "//       | Lifetime distribution: $F_DIST" >> $MODEL;
for (( i=1 ; i<=$N ; i++ )); do
	echo "" >> $MODEL;
	echo "" >> $MODEL;
	echo "module Node$i" >> $MODEL;
	echo "	N${i}clk: clock;          // Failure ~ $F_DIST" >> $MODEL;
	echo "	N${i}: bool init false;   // Node failed?" >> $MODEL;
	echo "	[f$i!] !N$i @ N${i}clk -> (N$i'= true);" >> $MODEL;
	echo "	[r$i?]  N$i          -> (N$i'= false) & (N${i}clk'= $F_DIST);" >> $MODEL;
	echo "endmodule" >> $MODEL;
done
echo "" >> $MODEL;

# REPAIRMAN
echo "" >> $MODEL;
echo "" >> $MODEL;
echo "///////////////////////////////////////////////////////////////////////" >> $MODEL;
echo "//" >> $MODEL;
echo "// Repairman | Fixes first broken node found" >> $MODEL;
echo "//           | Repair-time distribution: $R_DIST" >> $MODEL;
echo "" >> $MODEL;
echo "" >> $MODEL;
echo "module Repairman" >> $MODEL;
echo "" >> $MODEL;
echo "	Rclk: clock;    // Repair ~ $R_DIST" >> $MODEL;
echo "	fix : [0..$N];  // Which node are we fixing now" >> $MODEL;
#for i in $(seq $N); do echo "	N${i}f : bool init false;  // Node $i failed?"; done >> $MODEL;
for (( i=1 ; i<=$N ; i++ )); do
	echo "	N${i}f : bool init false;  // Node $i failed?" >> $MODEL;
done
echo "" >> $MODEL;
echo "	// Repair a failed node now" >> $MODEL;
for (( i=1 ; i<=$N ; i++ )); do
	echo "	[f$i?] fix == 0 -> (N${i}f'= true) & (fix' = $i) & (Rclk'= $R_DIST);" >> $MODEL;
done
echo "" >> $MODEL;
echo "	// Register a failed node for later repair" >> $MODEL;
for (( i=1 ; i<=$N ; i++ )); do
	echo "	[f$i?] fix > 0 -> (N${i}f'= true);" >> $MODEL;
done
echo "" >> $MODEL;
echo "	// Report a repair of a node and seek the next one" >> $MODEL;
for (( i=1 ; i<=$N ; i++ )); do
for (( k=1 ; k<=$N ; k++ )); do
	if [ $i -eq $k ]; then continue; fi
	echo "	[r$i!] fix == $i & N${k}f" >> $MODEL;
	for (( j=1 ; j<$k ; j++ )); do
		if [ $j -ne $i ]; then
			echo "	       & !N${j}f" >> $MODEL;
		fi
	done
	echo "	       @ Rclk -> (N${i}f'= false) & (fix'= $k) & (Rclk'= $R_DIST);" >> $MODEL;
done
done
echo "" >> $MODEL;
echo "	// Report a repair of last failed node and go to sleep" >> $MODEL;
for (( i=1 ; i<=$N ; i++ )); do
	echo "	[r$i!] fix == $i" >> $MODEL;
	for (( j=1 ; j<=$N ; j++ )); do
		if [ $j -ne $i ]; then
			echo "	       & !N${j}f" >> $MODEL;
		fi
	done
	echo "	       @ Rclk -> (N${i}f'= false) & (fix'= 0);" >> $MODEL;
done
echo "endmodule" >> $MODEL;
echo "" >> $MODEL;

# PROPERTY
echo "" >> $MODEL;
echo "properties" >> $MODEL;
echo -en "  S(" >> $MODEL;
for (( i=1 ; i <= N-K+1 ; i++ )); do
	echo -en "\n    (" >> $MODEL;
	for (( j=i ; j < i+K ; j++ )); do >> $MODEL;
		echo -en "N$j & " >> $MODEL;
	done;
	echo -en "true) |" >> $MODEL;
done;
echo " false" >> $MODEL;
echo "   ) // \"rate\"" >> $MODEL;
echo "endproperties" >> $MODEL;
echo "" >> $MODEL;

exit 0

