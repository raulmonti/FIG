#!/bin/bash
#
# Author:  Carlos E. Budde
# Date:    03.05.2015
# License: GPLv3
#

set -e

# Find resources
CMAKE_DIR=../../tests/estimations/
if [ ! -f ${CMAKE_DIR}CMakeLists.txt ]
then
	echo "[ERROR] Couldn't find CMakeLists.txt file in $CMAKE_DIR, aborting."
	exit 1
fi

# Build project
BUILD_DIR=bin
EXE="$BUILD_DIR/tests/tests_estimations"
if [ ! -d $BUILD_DIR ]; then mkdir $BUILD_DIR; fi
cd $BUILD_DIR && CC=gcc CXX=g++ cmake ../$CMAKE_DIR && make && cd ..
if [ ! -f $EXE ]
then
	echo "[ERROR] Couldn't build project, aborting."
	exit 1
fi
FIG="./fig"
ln -s $EXE $FIG

# Prepare testing environment
RESULTS="results"
if [ ! -d $RESULTS ]
then
	mkdir $RESULTS
else
	echo "[ERROR] Results dir already exists, aborting."
	exit 1
fi
PROPS_FILE="tandem_queue.pp"
echo 'P( q2 > 0 U lost)' > $PROPS_FILE

# Launch experiments
for (( c = 8 ; c <= 14 ; c += 2 ))  # queue capacities
do
	for s in {2,3,5,9}  # splitting
	do
		echo -n "Experiments for capacity $c and splitting ${s}..."
		log.out=${RESULTS}/tandem_queue_c${c}_s${s}.out
		log.err=${RESULTS}/tandem_queue_c${c}_s${s}.err
		$FIG tandem_queue_${c}.sa tandem_queue.pp $s 1>$log.out 2>$log.err
		mv $log.out $log.err $RESULTS
		echo " done"
	done
done

exit 0

