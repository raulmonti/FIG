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

### Job queue to use
### Available in Argentina clusters: mono, gpu, multi, phi
### Available in UT clusters: main, debug, m610, t630, r730
#SBATCH --partition=main

### Amount of nodes to use
#SBATCH --nodes=1

### Processes per node
#SBATCH --ntasks-per-node=1

### Available cores per node
#SBATCH --cpus-per-task=24
export MAXJOBSN=24  # must equal value of "--cpus-per-task"

### Execution time. Format: days-hours:minutes:seconds -- Max: a week
### XXX  DEPRECATED!  Should have been set with "-t" option
### XXX               when called as argument to enqueue_job.sh
#SBATCH --time 4-12:00

### Check invocation line
if [ $# -ne 1 ] || [ ! -f $1 ]; then
	echo "[ERROR] Must invoke through \"enqueue_job.sh\"";
	exit 1;
fi

## Load environment modules
set +e;
module load gcc;
module load bison;
#source /opt/spack/share/spack/setup-env.sh
#module load compilers/gcc/4.9
#module load gcc-5.4.0-gcc-4.4.7-xtrdik54ugxvc5fs55kl7lobaguiorqj

### Enqueue job
CWD=$PWD;
cd `dirname $1`;
srun -o %j.out -e %j.err /bin/bash `basename $1`;
cd $CWD;

exit 0;

