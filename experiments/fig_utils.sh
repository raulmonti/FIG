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
is_file()
{
	if [ ! -f "$1" ]
	then
		echo "[ERROR] Couldn't find file \"$1\"" 1>&2
		exit 1
	else
		echo "File \"$1\" found"
	fi
}


# Build main FIG project and link binary into specified (absolute) path
build_fig()
{
	# Check arguments
	if [ $# -lt 1 ]
	then
		echo "[ERROR] Must provide absolute path to link FIG's binary into"
		return 1
	elif [ ! -d $1 ]
	then
		echo "[ERROR] \"$1\" isn't a valid path"
		return 1
	elif [[ "$1" != /* ]]
	then
		echo "[ERROR] An *absolute* path for linking must be given"
		return 1
	fi
	is_file "${BASE_DIR}/CMakeLists.txt"   >/dev/null
	is_file "${BASE_DIR}/build_project.sh" >/dev/null
	# Build if need be
	CWD=$PWD
	cd $BASE_DIR
	if [ ! -d bin ]
	then
		./build_project.sh main
	elif [ ! -f bin/fig/fig ]
	then
		if [ -d OLD_bin ]; then rm -rf OLD_bin; fi; mv bin OLD_bin
		./build_project.sh main
	elif `bin/fig/fig --help &> /dev/null` [ $? -ne 0 ]
	then
		rm -rf bin
		./build_project.sh main
	fi
	cd $CWD
	if [ ! -f $BASE_DIR/bin/fig/fig ]
	then
		echo "[ERROR] Couldn't build FIG project."
		return 1
	fi
	# Link where requested
	if [ -f $1/fig ]; then rm $1/fig; fi
	ln -s $BASE_DIR/bin/fig/fig $1/fig
}


# Copy some SA model file from the FIG project into specified (absolute) path
copy_model_file()
{
	# Check arguments
	if [ $# -lt 1 ]
	then
		echo "[ERROR] Must provide the name of the model file to copy"
		return 1
	elif [ $# -lt 2 ]
	then
		echo "[ERROR] Must provide a path to copy the model into"
		return 1
	elif [ ! -d $2 ]
	then
		echo "[ERROR] \"$2\" isn't a valid directory"
		return 1
	elif [[ "$2" != /* ]]
	then
		echo "[ERROR] An *absolute* path to copy the model into must be given"
		return 1
	fi
	# Look for the model file
	local MODELS_DIR="$BASE_DIR/models"
	if [ ! -d $MODELS_DIR ]
	then
		echo "[ERROR] Couldn't find models directory \"$MODELS_DIR\""
		return 1
	elif [ ! -f $MODELS_DIR/$1 ]
	then
		echo "[ERROR] Couldn't find model file \"$1\" in dir \"$MODELS_DIR\""
		return 1
	fi
	# Copy the file into the requested path
	cp $MODELS_DIR/$1 $2
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

