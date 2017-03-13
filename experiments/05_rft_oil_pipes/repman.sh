# $1 : unique name for the repman
# $2 : number of inputs
# $3 : list of names of inputs



echo module repman_${1}
echo "    xs_${1}[${2}] : bool init false;"
echo "    busy_${1} : bool init false;"
echo
j=$((0))
for i in $3; do
    echo "    [ fail_${i}?? ] -> (xs_${1}[$j]' = true);"
    j=$((${j}+1));
done
echo
j=$((0))
for i in $3; do
    echo "    [ repair_${i}!! ] busy_${1} == false & fsteq(xs_${1},true) == ${j} -> (busy_${1}' = true);";
    j=$((${j}+1));
done
echo
j=$((0))
for i in $3; do
    echo "    [ repaired_${i}?? ] -> (busy_${1}' = false) & (xs_${1}[$j]' = false );"
    j=$((${j}+1));
done
echo
echo endmodule
