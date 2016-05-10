#!/bin/bash
#
# Author:  Carlos E. Budde
# Date:    23.03.2016
# License: GPLv3
#
#
#   Don't invoke this script directly,
#   run through "enqueue_job.sh" instead.
#
#
### Lines "#SBATCH" configure the job resources
### (even though they look like bash comments)

### Job queue to use (options: capacity, capability, gpu)
#SBATCH --partition=capacity

### Amount of nodes to use
#SBATCH --nodes=1

### Processes per node
#SBATCH --ntasks-per-node=1

### Available cores per node
#SBATCH --cpus-per-task=8

### execution time. Format: days-hours:minutes. Max: four days
#SBATCH --time 4-00:00

### Check invocation line
if [ $# -ne 1 ] || [ ! -f $1 ]
then
	echo "[ERROR] Must invoke through \"enqueue_job.sh\""
	exit 1
fi

## Load environment modules
module load compilers/gcc/4.9
#module load smtsolvers/z3

### Enqueue job
CWD=$PWD
cd `dirname $1`
srun -o %j.out -e %j.err /bin/bash `basename $1`
cd $CWD

exit 0

