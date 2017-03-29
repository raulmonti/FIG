#$1=unique name for this module
#$2=fail clock distribution
#$3=repair clock distribution


echo "module BE_${1}"
echo
echo "    c_fail_${1}: clock;"
echo "    c_repair_${1}: clock;"
echo "    inform_${1}: [0..2] init 0; // 0 not inform${1}, 1 inf. fail, 2 inf. repair"
# broken distingue entre roto y siendo reparado porque en el estado inicial se le
# da un valir inicial a c_repair, y no queremos que se tome la transicion 
# [] @c_repair antes de que en efecto se rompa la BE y ademas se empiece a reparar
# a partir de la orden de la RBOX. 
echo "    broken_${1}: [0..2] init 0; // 0 not broken, 1 broken, 2 being fixed"
echo ""
echo "    // failing (by itself)"
echo "    [f${1}!] broken_${1}==0 & inform_${1}==0 @ c_fail_${1} -> (inform_${1}'=1) & (broken_${1}'=1);"
echo ""
echo "    [fail_${1}!!] inform_${1} == 1 -> (inform_${1}'=0);"
echo
echo "    // reparation (with repairman)"
echo "    [repair_${1}??] broken_${1}==1 & inform_${1}==0 -> (broken_${1}'=2) & (c_repair_${1}'=${3});"
echo "    [r${1}!] broken_${1} == 2 @ c_repair_${1} -> (inform_${1}'=2) & (c_fail_${1}'=${2}) & (broken_${1}'=0);"
echo "    [repaired_${1}!!] inform_${1} == 2 -> (inform_${1}'=0);"
echo
echo "endmodule"
