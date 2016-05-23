#!/bin/bash
#
# Author:  Carlos E. Budde
# Date:    25.03.2016
# License: GPLv3
#

### Check invocation line
if [ $# -eq 0 ]
then
	echo "Usage: $0 [-d] <main_script_to_run>"
	echo "Available scripts are:"
	for script in `find . -name "main.sh"`; do
		/bin/echo -e "\t${script}"
	done
	echo "Option '-d' runs the script in the debug queue"
	exit 1
elif [ $# -eq 1 ] && [[ $1 == "--help" || $1 == "-h" ]]
then
	echo "Usage: $0 [-d] <main_script_to_run>"
	echo "Available scripts are:"
	for script in `find . -name "main.sh"`; do
		/bin/echo -e "\t${script}"
	done
	echo "Option '-d' runs the script in the debug queue"
	exit 0
elif [ $# -lt 1 ]
then
	echo "[ERROR] Must call with one argument: main script to run"
	exit 1
fi
if [ $# -eq 1 ]
then
	FILENAME=$1
elif [[ "$1" == "-d" ]]
then
	FILENAME=$2
else
	echo "Unrecognized option '$1'"
	echo "Usage: $0 [-d] <main_script_to_run>"
	echo "Option '-d' runs the script in the debug queue"
	exit 1
fi
if [ ! -f $FILENAME ]
then
	echo "[ERROR] Script file \"$FILENAME\" not found"
	exit 1
else
	NAME=`echo ${FILENAME#./} | cut -d'_' -f 1`"_"
	NAME+=`echo ${FILENAME#./} | cut -d'/' -f 2`
fi

# Allocate job
if [[ "$1" == "-d" ]]
then
	sbatch -J ${NAME} debug_job_slurm.sh "$FILENAME"
else
	sbatch -J ${NAME} submit_job_slurm.sh "$FILENAME"
fi

exit 0

