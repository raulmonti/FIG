#$1=unique name for this module


echo module BE_${1}
echo
echo "    c_enabled_${1}: clock;"
echo "    c_dormant_${1}: clock;"
echo "    c_repair_${1}: clock;"
echo "    inform_${1}: [0..2] init 0; // 0 not inform${1}, 1 inf. fail, 2 inf. repair"
echo "    enabled_${1}: bool init true;"
# broken distingue entre roto y siendo reparado porque en el estado inicial se le
# da un valir inicial a c_repair, y no queremos que se tome la transicion 
# [] @c_repair antes de que en efecto se rompa la BE y ademas se empiece a reparar
# a partir de la orden de la RBOX. 
echo "    broken_${1}: [0..2] init 0; // 0 not broken, 1 broken, 2 being fixed"
echo
echo "    // enabling - disabling"
echo "    [enable_${1}??] !enabled_${1} -> (enabled_${1}'=true) & (c_enabled_${1}'=exponential(0.001));"
echo "    [disable_${1}??] enabled_${1} -> (enabled_${1}'=false) & (c_dormant_${1}'=exponential(0.001));"
echo
echo "    // failing (by itself)"
echo "    [] enabled_${1} & broken_${1}==0 & inform_${1}==0 @ c_enabled_${1} -> (inform_${1}'=1) & (broken_${1}'=1);"
echo "    [] !enabled_${1} & broken_${1}==0 & inform_${1}==0 @ c_dormant_${1} -> (inform_${1}'=1) & (broken_${1}'=1);"
echo "    [fail_${1}!!] inform_${1} == 1 -> (inform_${1}'=0);"
echo
echo "    // reparation (with repairman)"
echo "    [repair_${1}??] broken_${1}==1 & inform_${1}==0 -> (broken_${1}'=2) & (c_repair_${1}'=lognormal(1.21,0.64));"
echo "    [] broken_${1} == 2 & enabled_${1} @ c_repair_${1} -> (inform_${1}'=2) & (c_enabled_${1}'=exponential(0.001)) & (broken_${1}'=0);"
echo "    [] broken_${1} == 2 & !enabled_${1} @ c_repair_${1} -> (inform_${1}'=2) & (c_dormant_${1}'=exponential(0.001)) & (broken_${1}'=0);"
echo "    [repaired_${1}!!] inform_${1} == 2 -> (inform_${1}'=0);"
echo
echo endmodule
