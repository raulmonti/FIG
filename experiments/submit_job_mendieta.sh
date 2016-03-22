#!/bin/bash
#
# Author:  Carlos E. Budde
# Date:    03.05.2015
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
#SBATCH --time 1-00:00

### Environment modules
module load compilers/gcc/4.9

### Lanzado de la tarea
srun /bin/bash test_main.sh

exit 0

