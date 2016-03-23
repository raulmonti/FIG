#!/bin/bash
#
# Author:  Carlos E. Budde
# Date:    22.03.2016
# License: GPLv3
#
# NOTE: this script should be sourced from another script
#


# Following assumes this script file is one level deeper than the project base
BASE_DIR=`readlink -f "$(dirname ${BASH_SOURCE[0]})/.."`
if [ ! -d $BASE_DIR ]
then
	echo "[ERROR] Couldn't find project's base directory"
	return 1
fi

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
	is_file "${BASE_DIR}/CMakeLists.txt" >/dev/null
	if [ -d $1 ]; then rm -rf $1; fi
	CWD=$PWD
	mkdir $1 && cd $1
	CC=gcc CXX=g++ cmake $BASE_DIR && make
	cd $CWD
	if [ ! -f $1/fig/fig ]
	then
		echo "[ERROR] Couldn't build project."
		return 1
	else
		echo "Project successfully built in $1"
	fi
}

# Make a local copy of some SA model file from the FIG project
copy_model_file() {
	if [ $# -lt 1 ]
	then
		echo "[ERROR] Must call with the name of the mode file to copy"
		return 1
	fi
	local MODELS_DIR="$BASE_DIR/models"
	if [ ! -d $MODELS_DIR ]
	then
		echo "[ERROR] Couldn't find models directory \"$MODELS_DIR\""
		return 1
	elif [ ! -f $MODELS_DIR/$1 ]
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
	# do-while syntax sugar for bash
	while
		local RUNNING=`pgrep -u "$(whoami)" fig | wc -l`
		[ $RUNNING -ge $JOBSBOUND ]
	do
		sleep 10s
	done
}

return 0

