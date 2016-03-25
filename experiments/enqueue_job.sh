#!/bin/bash
#
# Author:  Carlos E. Budde
# Date:    25.03.2016
# License: GPLv3
#

### Check invocation line
if [ $# -eq 0 ]
then
	echo "Usage: $0 <main_script_to_run>"
	echo "Available scripts are:"
	for script in `find . -name "main.sh"`; do
		/bin/echo -e "\t${script}"
	done
	exit 1
elif [ $# -eq 1 ] && [[ $1 == "--help" || $1 == "-h" ]]
then
	echo "Usage: $0 <main_script_to_run>"
	echo "Available scripts are:"
	for script in `find . -name "main.sh"`; do
		/bin/echo -e "\t${script}"
	done
	exit 0
elif [ $# -ne 1 ]
then
	echo "[ERROR] Must call with one argument: main script to run"
	exit 1
elif [ ! -f $1 ]
then
	echo "[ERROR] Script file \"$1\" not found"
	exit 1
else
	NAME=`echo $1 | cut -d'_' -f 1`"_"
	NAME+=`echo $1 | cut -d'/' -f 2`
fi

# Allocate job
sbatch -J ${NAME} submit_job_slurm.sh "$1"

exit 0

