# $1: Nombre unico de la compuerta
# $2: numero de nodos conectados a la compuerta
# $3: 3[] names of the 2 3 to the and gate


echo module AND${1}
echo
echo "    k_${1}: [0 .. ${2}] init 0;"
echo "    inform_${1} : bool init false;"
echo
for i in $3; do
    echo "    [ fail_${i}?? ] -> (k_${1}' = k_${1}+1) & (inform_${1}' = (k_${1}==$((${2}-1))));"
done

echo "    [ fail_${1}!! ] inform_${1} & k_${1} == 2 -> (inform_${1}' = false);"
echo
for i in $3; do
    echo "    [ repair_${i}?? ] -> (k_${1}' = k_${1}-1) & (inform_${1}' = (k_${1}==${2}));"
done
echo
echo "    [ repair_${1}!! ] inform_${1} & k_${1} < ${2} -> (inform_${1}' = false);"
echo
echo endmodule
