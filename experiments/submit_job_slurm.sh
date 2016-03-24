#!/bin/bash
#
# Author:  Carlos E. Budde
# Date:    23.03.2016
# License: GPLv3
#
### Lines "#SBATCH" configure the job resources
### (even though they look like bash comments)

### Job outputs
#SBATCH 

### Job queue to use (options: capacity, capability, gpu)
#SBATCH --partition=capacity

### Amount of nodes to use
#SBATCH --nodes=1

### Processes per node
#SBATCH --ntasks-per-node=1

### Available cores per node
#SBATCH --cpus-per-task=8

### execution time. Format: days-hours:minutes. Max: four days
#SBATCH --time 2-00:00

### Check invocation line
if [ $# -lt 1 ]
then
	echo "[ERROR] Must call with one argument: main script to run"
	exit 1
elif [ ! -f $1 ]
then
	echo "[ERROR] Script file \"$1\" not found"
	exit 1
else
	NAME=`echo $1 | cut -d'_' -f 1`"_"
	NAME+=`echo $1 | cut -d'/' -f 2`"_"
fi

## Load environment modules
#module load compilers/gcc/4.9
#module load smtsolvers/z3

### Enqueue job
CWD=$PWD
cd `dirname $1`
#sbatch -o %j.out -e %j.err -J ${NAME}%j  # allocate resources and configure job
#srun /bin/bash `basename $1`             # launch job
/bin/bash `basename $1`             # launch job
cd $CWD

exit 0

