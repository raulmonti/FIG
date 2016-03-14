#!/bin/bash
#
# Author:  Carlos E. Budde
# Date:    03.05.2015
# License: GPLv3
#

set -e

# Find resources
CMAKE_DIR=../tests/estimations/
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

# Launch experiments
SPLIT="2"
RESULTS="results"
if [ ! -d $RESULTS ]; then mkdir $RESULTS; fi
for (( c = 8 ; c <= 14 ; c += 2 ))
do
	echo "Experiments for tandem queue with capacity $c"
	$FIG tandem_queue_${c}.sa tandem_queue.pp $SPLIT   \
		1> ${RESULTS}/tandem_queue_c${c}_s${SPLIT}.out \
		2> ${RESULTS}/tandem_queue_c${c}_s${SPLIT}.err &
done

exit 0

