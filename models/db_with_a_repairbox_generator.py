#!/usr/bin/python

# -*- coding: utf-8 -*- 

from string import Template
import os

disk_template = """module Disk_${MOD}_${NUMBER}

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

"""
disk_template = Template(disk_template)

cont_template = Template("""module Controller_${MOD}_${NUMBER}

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

proc_template = Template("""module Processor_${MOD}_${NUMBER}

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


disk_modules = ['A', 'B', 'C', 'D', 'E', 'F']
disks_for_modules = 4

control_modules = ['A', 'B']
controllers_for_modules = 2

process_modules = ['A', 'B']
processors_for_modules = 2

MEAN_TIME_TO_FAILURE = 2500

proc_mttf = MEAN_TIME_TO_FAILURE
cont_mttf = MEAN_TIME_TO_FAILURE
disk_mttf = 3 * MEAN_TIME_TO_FAILURE

rep_par_1 = 1.0
rep_par_2 = 0.5
bkn_par = 1.0/(disk_mttf *2)

data = {}
data['REP_PAR_1'] = rep_par_1
data['REP_PAR_2'] = rep_par_2
data['BKN_PAR'] = "{:.8f}".format(bkn_par)

for module in disk_modules:
	for number in range(0, disks_for_modules): 
		data['MOD'] = module
		data['NUMBER'] = number
		print disk_template.substitute(data)


bkn_par = 1.0/(cont_mttf *2)
data['BKN_PAR'] = "{:.8f}".format(bkn_par)
for module in control_modules: 
	for number in range(0, controllers_for_modules):
		data['MOD'] = module
		data['NUMBER'] = number
		print cont_template.substitute(data)

bkn_par = 1.0/(proc_mttf *2)
data['BKN_PAR'] = "{:.8f}".format(bkn_par)
for module in process_modules: 
	for number in range(0, processors_for_modules):
		data['MOD'] = module
		data['NUMBER'] = number
		print proc_template.substitute(data)

be_number = str(  len(disk_modules) * disks_for_modules 
				+ len(control_modules) * controllers_for_modules 
				+ len(process_modules) * processors_for_modules)  
print "module repairbox"
print "	idle : bool init true;"
print "	request ["+  be_number +"] : bool init false;"

be_id = 0

trans_for_disk = Template("""
	[ disk_${MOD}_${NUMBER}_fail? ] -> (request[${BE_ID}]' = true);
	[ disk_${MOD}_${NUMBER}_repair!! ] idle & rndeq(request, true) == ${BE_ID} -> (request[${BE_ID}]' = false) & (idle' = false);
	[ disk_${MOD}_${NUMBER}_repaired? ] idle == false -> (idle' = true);""")

for module in disk_modules:
	for number in range(0, disks_for_modules):
		data['MOD'] = module
		data['NUMBER'] = number
		data['BE_ID'] = be_id 
		print trans_for_disk.substitute(data)
		be_id += 1 

trans_for_controller = Template("""
	[ controller_${MOD}_${NUMBER}_fail? ] -> (request[${BE_ID}]' = true);
	[ controller_${MOD}_${NUMBER}_repair!! ] idle & rndeq(request, true) == ${BE_ID} -> (request[${BE_ID}]' = false) & (idle' = false);
	[ controller_${MOD}_${NUMBER}_repaired? ] idle == false -> (idle' = true);""")

for module in control_modules:
	for number in range(0, controllers_for_modules):
		data['MOD'] = module
		data['NUMBER'] = number
		data['BE_ID'] = be_id 
		print trans_for_controller.substitute(data)
		be_id += 1 

trans_for_processor = Template("""
	[ processor_${MOD}_${NUMBER}_fail? ] -> (request[${BE_ID}]' = true);
	[ processor_${MOD}_${NUMBER}_repair!! ] idle & rndeq(request, true) == ${BE_ID} -> (request[${BE_ID}]' = false) & (idle' = false);
	[ processor_${MOD}_${NUMBER}_repaired? ] idle == false -> (idle' = true);""")

for module in process_modules:
	for number in range(0, processors_for_modules):
		data['MOD'] = module
		data['NUMBER'] = number
		data['BE_ID'] = be_id 
		print trans_for_processor.substitute(data)
		be_id += 1 

print "endmodule"

def disk_prop():
	disks = {}
	for module in disk_modules:
		disks[module] = []
		for number in range(0, disks_for_modules):
			disks[module].append((module, number))
	
	template = Template("    (dstatus_${M1}_${N1} > 0 & dstatus_${M2}_${N2} > 0) |")
	prop = ""
	data = {}
	for module in disk_modules:
		for i in range(0, len(disks[module])-1):
			(data['M1'],data['N1']) = disks[module][i]
			for j in range(i+1, len(disks[module])): 
				(data['M2'],data['N2']) = disks[module][j]
				prop += template.substitute(data) + os.linesep
	return prop

def cont_prop():
	controllers = {}
	for module in control_modules:
		controllers[module] = []
		for number in range(0, controllers_for_modules):
			controllers[module].append((module, number))

	template = Template("(cstatus_${M1}_${N1} > 0) & ")	
	prop = ""
	data = {}
	for module in control_modules:
		prop += "    "
		for i in range(0, len(controllers[module])):
			(data['M1'],data['N1']) = controllers[module][i]
			prop += template.substitute(data)
		prop += "true | " + os.linesep 
	return prop

def proc_prop():
	processors = {}
	for module in process_modules:
		processors[module] = []
		for number in range(0, processors_for_modules):
			processors[module].append((module, number))

	template = Template("(pstatus_${M1}_${N1} > 0) & ")	
	prop = ""
	data = {}
	for module in process_modules:
		prop += "    "
		for i in range(0, len(processors[module])):
			(data['M1'],data['N1']) = processors[module][i]
			prop += template.substitute(data)
		prop += "true | " + os.linesep 
	return prop

print """properties
    S ( """
print disk_prop()
print cont_prop()
print proc_prop()
print """    false ) 
endproperties"""

