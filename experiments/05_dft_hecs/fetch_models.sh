#!/usr/bin/env bash
#
# Author:  Carlos E. Budde
# Date:    04.12.2017
# License: GPLv3
#
#   Export array MODEL_LIST[*] with all models for experimentation
#   NOTE: this script should be sourced from another script
#
declare -a MODEL_LIST
MODEL_LIST=(\
	'hecs_1_1_0_np_with_repairs.sa'
	'hecs_1_1_0_up_with_repairs.sa'
	'hecs_1_1_1_np_with_repairs.sa'
	'hecs_1_1_2_up_with_repairs.sa'
	'hecs_2_1_0_np_with_repairs.sa'
	'hecs_3_1_0_np_with_repairs.sa'
	'hecs_4_1_0_np_with_repairs.sa'
	'hecs_5_1_1_np_with_repairs.sa'
	'hecs_6_1_1_np_with_repairs.sa'
)
export MODEL_LIST
return 0
