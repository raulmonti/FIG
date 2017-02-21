#$1=unique name for this module


echo module BE_${1}
echo
echo "    c_enabled_${1}: clock;"
echo "    c_dormant_${1}: clock;"
echo "    c_repair_${1}: clock;"
echo "    inform_${1}: [0..2] init 0; // 0 not inform${1}, 1 inf. fail, 2 inf. repair"
echo "    enabled_${1}: bool init true;"
echo "    broken_${1}: bool init false;"
echo
echo "    // enabling - disabling"
echo "    [enable_${1}??] !enabled_${1} -> (enabled_${1}'=true) & (c_enabled_${1}'=exponential(0.001));"
echo "    [disable_${1}??] enabled_${1} -> (enabled_${1}'=false) & (c_dormant_${1}'=exponential(0.001));"
echo
echo "    // failing (by itself)"
echo "    [] enabled_${1} & !broken_${1} @ c_enabled_${1} -> (inform_${1}'=1) & (broken_${1}'=true);"
echo "    [] !enabled_${1} & !broken_${1} @ c_dormant_${1} -> (inform_${1}'=1) & (broken_${1}'=true);"
echo "    [fail_${1}!!] inform_${1} == 1 -> (inform_${1}'=0);"
echo
echo "    // reparation (with repairman)"
echo "    [repair_${1}??] -> (c_repair_${1}'=lognormal(1.21,0.8));"
echo "    [] enabled_${1} @ c_repair_${1} -> (inform_${1}'=2) & (c_enabled_${1}'=exponential(0.001)) & (broken_${1}'=false);"
echo "    [] !enabled_${1} @ c_repair_${1} -> (inform_${1}'=2) & (c_dormant_${1}'=exponential(0.001))  & (broken_${1}'=false);"
echo "    [repaired_${1}!!] inform_${1} == 2 -> (inform_${1}'=0);"
echo
echo endmodule
