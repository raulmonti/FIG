#!/bin/bash
#
# Author:  Carlos E. Budde
# Date:    22.03.2016
# License: GPLv3
#
# NOTE: this script should be sourced from another script
#


# Check argument is a regular file
is_file() {
	if [ ! -f "$1" ]
	then
		echo "[ERROR] Couldn't find file \"$1\"" 1>&2
		exit 1
	else
		echo "File \"$1\" found"
	fi
}

# Build (main) FIG project in specified path
build_fig() {
	if [ $# -lt 1 ]
	then
		echo "[ERROR] Must call with the path where FIG is to be built"
		return 1
	fi
	local BASE_DIR=`realpath ..`
	is_file "${BASE_DIR}/CMakeLists.txt" 1>/dev/null
	if [ -d $1 ]; then rm -rf $1; fi; mkdir $1
	CWD=$PWD
	cd $1; CC=gcc CXX=g++ cmake $BASE_DIR && make; cd $CWD
	if [ ! -f $1/fig/fig ]
	then
		echo "[ERROR] Couldn't build project."
		return 1
	else
		echo "Project successfully built in $1"
	fi
}

# Make a local copy of some SA model file
copy_model_file() {
	if [ $# -lt 1 ]
	then
		echo "[ERROR] Must call with the name of the mode file to copy"
		return 1
	fi
	local MODELS_DIR=`realpath ../models`
	if [ ! -f $MODELS_DIR/$1 ]
	then
		echo "[ERROR] Couldn't find model file \"$1\" in dir \"$MODELS_DIR\""
		return 1
	else
		cp $MODELS_DIR/$1 ./
	fi
}

# Poll until less than MAXJOBSN jobs called 'fig' are running
poll_till_free() {
	if [ -z "$MAXJOBSN" ]
	then
		local JOBSBOUND=`nproc --all`
	else
		local JOBSBOUND=$MAXJOBSN
	fi
	local user=`whoami`
	# do-while syntax sugar for bash
	while
		local RUNNING=`pgrep -u $user fig | wc -l`
		[ $RUNNING -ge $JOBSBOUND ]
	do
		sleep 10s
	done
}

return 0

