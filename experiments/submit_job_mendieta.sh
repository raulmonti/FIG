#!/bin/bash
#
# Author:  Carlos E. Budde
# Date:    23.03.2016
# License: GPLv3
#
### Las líneas #SBATCH configuran los recursos de la tarea
### (aunque parezcan estar comentadas)

### Cola de trabajos a usar (opciones: capacity, capability, gpu)
#SBATCH --partition=capacity

### Nombre de la tarea
#SBATCH --job-name=fig

### Cantidad de nodos a usar
#SBATCH --nodes=1

### Procesos por nodo
#SBATCH --ntasks-per-node=1

### Cores visibles por nodo
#SBATCH --cpus-per-task=8

### Tiempo de ejecución. Formato dias-horas:minutos. Máximo: cuatro días
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
fi

## Load environment modules
module load compilers/gcc/4.9
#module load smtsolvers/z3

### Enqueue task
CWD=$PWD
cd `dirname $1`
srun /bin/bash `basename $1`
cd $CWD

exit 0

