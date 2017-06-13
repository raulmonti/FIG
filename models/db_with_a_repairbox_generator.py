#!/usr/bin/python
# -*- coding: utf-8 -*- 

from string import Template
import os, sys, itertools

disk_template = Template("""module Disk${MOD}${NUMBER}

	dstatus_${MOD}_${NUMBER} : [0..4] init 0;  // 0: ok, 1: fail mode 1, 2: fail mode 2, 3: repairing mode 1, 4: repairing mode 2. 
	dfc1_${MOD}_${NUMBER} : clock;  // Type 1 failure ~ exp(1/(DF*2))
	dfc2_${MOD}_${NUMBER} : clock;  // Type 2 failure ~ exp(1/(DF*2))
	drc1_${MOD}_${NUMBER} : clock;  // Repair for type 1 failure ~ exp(1.0)
	drc2_${MOD}_${NUMBER} : clock;  // Repair for type 2 failure ~ exp(0.5)

	[ disk_${MOD}_${NUMBER}_fail! ] dstatus_${MOD}_${NUMBER} == 0 @ dfc1_${MOD}_${NUMBER} -> (dstatus_${MOD}_${NUMBER}'= 1);
	[ disk_${MOD}_${NUMBER}_fail! ] dstatus_${MOD}_${NUMBER} == 0 @ dfc2_${MOD}_${NUMBER} -> (dstatus_${MOD}_${NUMBER}'= 2);

	[ disk_${MOD}_${NUMBER}_repair?? ] dstatus_${MOD}_${NUMBER} == 1 -> (dstatus_${MOD}_${NUMBER}'= 3) & (drc1_${MOD}_${NUMBER}'= exponential(${REP_PAR_1})) ;
	[ disk_${MOD}_${NUMBER}_repair?? ] dstatus_${MOD}_${NUMBER} == 2 -> (dstatus_${MOD}_${NUMBER}'= 4) & (drc2_${MOD}_${NUMBER}'= exponential(${REP_PAR_2})) ;

	[ disk_${MOD}_${NUMBER}_repaired! ] dstatus_${MOD}_${NUMBER} == 3 @ drc1_${MOD}_${NUMBER} 
		-> (dstatus_${MOD}_${NUMBER}' = 0) & 
		(dfc1_${MOD}_${NUMBER}'= exponential(${BKN_PAR})) &
		(dfc2_${MOD}_${NUMBER}'= exponential(${BKN_PAR}));

	[ disk_${MOD}_${NUMBER}_repaired! ] dstatus_${MOD}_${NUMBER} == 4 @ drc2_${MOD}_${NUMBER} 
		-> (dstatus_${MOD}_${NUMBER}' = 0) &
		(dfc1_${MOD}_${NUMBER}'= exponential(${BKN_PAR})) &
		(dfc2_${MOD}_${NUMBER}'= exponential(${BKN_PAR}));

endmodule

""")

cont_template = Template("""module Controller${MOD}${NUMBER}

	cstatus_${MOD}_${NUMBER}: [0..4] init 0;  // 0: ok, 1: fail mode 1, 2: fail mode 2, 3: repairing mode 1, 4: repairing mode 2. 
	cfc1_${MOD}_${NUMBER} : clock;  // Type 1 failure ~ exp(1/(DF*2))
	cfc2_${MOD}_${NUMBER}: clock;  // Type 2 failure ~ exp(1/(DF*2))
	crc1_${MOD}_${NUMBER}: clock;  // Repair for type 1 failure ~ exp(1.0)
	crc2_${MOD}_${NUMBER}: clock;  // Repair for type 2 failure ~ exp(0.5)

	[ controller_${MOD}_${NUMBER}_fail! ] cstatus_${MOD}_${NUMBER} == 0 @ cfc1_${MOD}_${NUMBER} -> (cstatus_${MOD}_${NUMBER}'= 1);
	[ controller_${MOD}_${NUMBER}_fail! ] cstatus_${MOD}_${NUMBER} == 0 @ cfc2_${MOD}_${NUMBER} -> (cstatus_${MOD}_${NUMBER}'= 2);

	[ controller_${MOD}_${NUMBER}_repair?? ] cstatus_${MOD}_${NUMBER} == 1 
		-> (cstatus_${MOD}_${NUMBER}'= 3) 
		& (crc1_${MOD}_${NUMBER}'= exponential(${REP_PAR_1}));

	[ controller_${MOD}_${NUMBER}_repair?? ] cstatus_${MOD}_${NUMBER} == 2
		-> (cstatus_${MOD}_${NUMBER}'= 4) 
		& (crc2_${MOD}_${NUMBER}'= exponential(${REP_PAR_2}));

	[ controller_${MOD}_${NUMBER}_repaired! ] cstatus_${MOD}_${NUMBER} == 3 @ crc1_${MOD}_${NUMBER}
		-> (cstatus_${MOD}_${NUMBER}' = 0) 
		& (cfc1_${MOD}_${NUMBER}'= exponential(${BKN_PAR})) 
		& (cfc2_${MOD}_${NUMBER}'= exponential(${BKN_PAR}));

	[ controller_${MOD}_${NUMBER}_repaired! ] cstatus_${MOD}_${NUMBER} == 4 @ crc2_${MOD}_${NUMBER}
	 	-> (cstatus_${MOD}_${NUMBER}' = 0) 
	 	& (cfc1_${MOD}_${NUMBER}'= exponential(${BKN_PAR})) 
	 	& (cfc2_${MOD}_${NUMBER}'= exponential(${BKN_PAR}));

endmodule

""")

proc_template = Template("""module Processor${MOD}${NUMBER}

	pstatus_${MOD}_${NUMBER} 	: [0..4] init 0;       // 0: ok, 1: fail mode 1, 2: fail mode 2, 3: repairing mode 1, 4: repairing mode 2. 
	pfc1_${MOD}_${NUMBER} 		: clock;        // Type 1 failure ~ exp(1/(DF*3))
	pfc2_${MOD}_${NUMBER}		: clock;        // Type 2 failure ~ exp(1/(DF*3))
	prc1_${MOD}_${NUMBER} 		: clock;        // Repair for type 1 failure ~ exp(1.0)
	prc2_${MOD}_${NUMBER}		: clock;        // Repair for type 2 failure ~ exp(0.5)

	[ processor_${MOD}_${NUMBER}_fail! ] pstatus_${MOD}_${NUMBER} == 0 @ pfc1_${MOD}_${NUMBER} -> (pstatus_${MOD}_${NUMBER}'= 1);
	[ processor_${MOD}_${NUMBER}_fail! ] pstatus_${MOD}_${NUMBER} == 0 @ pfc2_${MOD}_${NUMBER} -> (pstatus_${MOD}_${NUMBER}'= 2);

	[ processor_${MOD}_${NUMBER}_repair?? ] pstatus_${MOD}_${NUMBER} == 1 -> (pstatus_${MOD}_${NUMBER}'= 3) & (prc1_${MOD}_${NUMBER}'= exponential(${REP_PAR_1})) ;
	[ processor_${MOD}_${NUMBER}_repair?? ] pstatus_${MOD}_${NUMBER} == 2 -> (pstatus_${MOD}_${NUMBER}'= 4) & (prc2_${MOD}_${NUMBER}'= exponential(${REP_PAR_2})) ;

	[ processor_${MOD}_${NUMBER}_repaired! ] pstatus_${MOD}_${NUMBER} == 3 @ prc1_${MOD}_${NUMBER} 
		-> (pstatus_${MOD}_${NUMBER}' = 0) 
		& (pfc1_${MOD}_${NUMBER}'= exponential(${BKN_PAR})) 
		& (pfc2_${MOD}_${NUMBER}'= exponential(${BKN_PAR}));

	[ processor_${MOD}_${NUMBER}_repaired! ] pstatus_${MOD}_${NUMBER} == 4 @ prc2_${MOD}_${NUMBER}
		-> (pstatus_${MOD}_${NUMBER}' = 0) 
		& (pfc1_${MOD}_${NUMBER}'= exponential(${BKN_PAR})) 
		& (pfc2_${MOD}_${NUMBER}'= exponential(${BKN_PAR}));

endmodule""")


begining_repair_box_template = Template("""
module repairbox
    idle : bool init true;"
    request [ ${SIZE} ] : bool init false;

""")

repair_transitions_4disk = Template("""
 	[ disk_${MOD}_${NUMBER}_fail? ] -> (request[${BE_ID}]' = true);
 	[ disk_${MOD}_${NUMBER}_repair!! ] idle & rndeq(request, true) == ${BE_ID} -> (request[${BE_ID}]' = false) & (idle' = false);
 	[ disk_${MOD}_${NUMBER}_repaired? ] idle == false -> (idle' = true);""")

repair_transitions_4controller = Template("""
 	[ controller_${MOD}_${NUMBER}_fail? ] -> (request[${BE_ID}]' = true);
 	[ controller_${MOD}_${NUMBER}_repair!! ] idle & rndeq(request, true) == ${BE_ID} -> (request[${BE_ID}]' = false) & (idle' = false);
 	[ controller_${MOD}_${NUMBER}_repaired? ] idle == false -> (idle' = true);""")

repair_transitions_4processor = Template("""
	[ processor_${MOD}_${NUMBER}_fail? ] -> (request[${BE_ID}]' = true);
	[ processor_${MOD}_${NUMBER}_repair!! ] idle & rndeq(request, true) == ${BE_ID} -> (request[${BE_ID}]' = false) & (idle' = false);
	[ processor_${MOD}_${NUMBER}_repaired? ] idle == false -> (idle' = true);""")

def print_repair_transitions(template, modules, components, initial_id): 
	data = {}
	for module in range(1, modules+1):
		for number in range(1, components+1):
			data['MOD'] = module
			data['NUMBER'] = number
			data['BE_ID'] = initial_id 
			print template.substitute(data)
			initial_id += 1 

	return initial_id

def print_module(template, modules, components, dict_initialized):
	for module in range(1, modules+1):
	 	for number in range(1, components+1): 
			dict_initialized['MOD'] = module
			dict_initialized['NUMBER'] = number
			print template.substitute(data)	

def or_of_ands_generator(or_template, and_template, proposition_template, modules, components):
	prop_with_and = Template(proposition_template + and_template)
	proposition = Template(proposition_template)
	p = ""
	for m in range(1, modules+1):
		p += "    "
		for c in range(1, components):
			(data['M'], data['D']) = (m, c)
			p += prop_with_and.substitute(data)
		(data['M'], data['D']) = (m, components)
		p += proposition.substitute(data) + or_template + os.linesep
	return p


if __name__ == '__main__':
	if len(sys.argv) != 6:
		print "USAGE: this_script.py <redundancy >= 2> <number_of_disk_modules> \
<number_of_control_modules> <number_of_process_modules> <root_mean_fail_time>"
		exit(1)
	
	redundancy = int(sys.argv[1])
	disks = 2 + redundancy
	controllers = redundancy
	processors = redundancy

	disk_modules = int(sys.argv[2])
	control_modules = int(sys.argv[3])
	process_modules = int(sys.argv[4])

	rmft = int(sys.argv[5])
	# Failure Time Calculations
	disk_failure_time = rmft * 3
	controller_failure_time = rmft
	processor_failure_time = rmft
	# Failure RATE Calculations
	NUM_FAIL_TYPE = 2 # our model has two types of failure. 
	disk_failure_rate = 1.0 / (NUM_FAIL_TYPE * disk_failure_time)
	controller_failure_rate = 1.0 / (NUM_FAIL_TYPE * controller_failure_time)
	processor_failure_rate = 1.0 / (NUM_FAIL_TYPE * processor_failure_time)

	data = {}
	data['REP_PAR_1'] = 1.0
	data['REP_PAR_2'] = 0.5
	data['BKN_PAR'] = "{:.8f}".format(disk_failure_rate)

	print_module(disk_template, disk_modules, disks, data)

	data['BKN_PAR'] = "{:.8f}".format(controller_failure_rate)
	print_module(cont_template, control_modules, controllers, data)	

	data['BKN_PAR'] = "{:.8f}".format(processor_failure_rate)
	print_module(proc_template, process_modules, processors, data)

	be_number = disk_modules * disks + control_modules*controllers + process_modules*processors 
	data['SIZE'] = be_number
	be_id = 0

	print begining_repair_box_template.substitute(data)
	be_id = print_repair_transitions(repair_transitions_4disk, disk_modules, disks, be_id)
	be_id = print_repair_transitions(repair_transitions_4controller, control_modules, controllers, be_id)
	be_id = print_repair_transitions(repair_transitions_4processor, process_modules, processors, be_id)
 	print "endmodule"

	# PRINT PROPERTY
	print """properties
    S ( """
	proposition_template = "dstatus_${M}_${D} > 0"
	and_template = " & "
	or_template = " | "
	
	prop_with_and = Template(proposition_template + and_template )
	proposition_template = Template(proposition_template)
	p = ""
	for m in range(1, disk_modules + 1): 
		module_disk = [(m,d) for d in range(1, disks+1)]
		for mytuple in itertools.combinations(module_disk, redundancy):
			p += "    "
			for i in range(0, len(mytuple)-1):
				(data['M'], data['D']) =  mytuple[i]
				p += prop_with_and.substitute(data)

			(data['M'], data['D']) =  mytuple[len(mytuple)-1]
			p += proposition_template.substitute(data) + or_template + os.linesep

	print p
	print or_of_ands_generator(" | ", " & ", "cstatus_${M}_${D} > 0", control_modules, controllers)
	print or_of_ands_generator(" | ", " & ", "pstatus_${M}_${D} > 0", process_modules, processors)
	print ("""    false )
endproperties""")

