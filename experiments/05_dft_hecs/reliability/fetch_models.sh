#
# Author:  Carlos E. Budde
# Date:    04.12.2017
# License: GPLv3
#
#   Export array MODEL_LIST[*] with all models for experimentation
#   NOTE: this script should be sourced from another script  XXX
#
declare -a MODEL_LIST
MODEL_LIST=(\
	'dft/hecs/hecs0/hecs_1_1_0_np_with_repairs.sa'
	'dft/hecs/hecs0/hecs_2_1_0_np_with_repairs.sa'
	'dft/hecs/hecs0/hecs_2_2_0_np_with_repairs.sa'
	'dft/hecs/hecs0/hecs_3_1_0_np_with_repairs.sa'
	'dft/hecs/hecs0/hecs_3_2_0_np_with_repairs.sa'
	'dft/hecs/hecs0/hecs_3_3_0_np_with_repairs.sa'
	'dft/hecs/hecs0/hecs_4_2_0_np_with_repairs.sa'
	'dft/hecs/hecs0/hecs_4_3_0_np_with_repairs.sa'
	'dft/hecs/hecs0/hecs_4_4_0_np_with_repairs.sa'
	'dft/hecs/hecs0/hecs_5_3_0_np_with_repairs.sa'
	'dft/hecs/hecs0/hecs_5_4_0_np_with_repairs.sa'
	'dft/hecs/hecs0/hecs_5_5_0_np_with_repairs.sa'
	'dft/hecs/hecs0/hecs_6_4_0_np_with_repairs.sa'
	'dft/hecs/hecs0/hecs_6_5_0_np_with_repairs.sa'
	'dft/hecs/hecs0/hecs_6_6_0_np_with_repairs.sa'
)
export MODEL_LIST
return 0
