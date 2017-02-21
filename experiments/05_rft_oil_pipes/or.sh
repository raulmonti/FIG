# $1 : unique name for this element
# $2 : number of inputs
# $3 : list of names of the inputs

echo module OR_${1}
echo "    k_${1} : [0..${2}] init 0;"
echo "    inform_${1} : bool init false;"
echo
for i in $3; do
echo "    [ fail_${i}?? ] -> (k_${1}' = k_${1}+1) & (inform_${1}' = (k_${1}==0));"
done
echo
echo "    [ fail_${1}!! ] inform_${1} & k_${1} != 0 -> (inform_${1}' = false);"
echo 
for i in $3; do
echo "    [ repair_${i}?? ] -> (k_${1}' = k_${1}-1) & (inform_${1}' = (k_${1}==${2}));"
done
echo
echo "    [ repair_${1}!! ] inform_${1} & k_${1} == 0 -> (inform_${1}' = false);"
echo
echo endmodule
