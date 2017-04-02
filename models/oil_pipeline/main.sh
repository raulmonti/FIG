# $1 : number of pipes
# $2 : consecutive broken pipes
# $3 : fail distribution
# $4 : repair distribution
# $5 : output file name for model (and properties)
# TODO enable seting consecutive broken pipes limit (now 4)

# Check calling arguments
if [ $# -ne 5 ]; then
	echo "[ERROR] Invalid invocation, call with 5 arguments:"
	echo "  1. Number of pipes                       (aka 'N', e.g. 60)"
	echo "  2. Number of consecutive broken pipes    (aka 'K', e.g.  4)"
	echo "  3. Fail distribution             (e.g. exponential(0.001) )"
	echo "  4. Repair distribution           (e.g. lognormal(1.21,.64))"
	echo "  5. Output filename               (e.g. \"oil_pipes_60_4.sa\")"
	exit 1
fi

# Clear the old model if any.
echo "" > ${5} 

# Macro
END=$((${1}-${2}+1))

#BUILD PIPES
for i in $(seq 1 ${1}); do
	bash be.sh pipe$i $3 $4 >> $5;
	echo -en "\n\n" >> $5;
done

#BUILD ANDS

#for i in $(seq 1 $END); do 
#    bash and.sh and$i 6 "pipe$((${i})) pipe$((${i}+1)) pipe$((${i}+2)) pipe$((${i}+3))"  >> $2 ;
#done

#BUILD THE REPAIRMAN
bes=""
for i in $(seq 1 ${1}); do
    bes=${bes}" pipe$i"
done
bash repman.sh "A" ${1} "$bes" >> $5;

#BUILD THE OR TOP NODE
#ands=""
#for i in $(seq 1 $END); do
#    ands=${ands}" and$i"
#done
#bash or.sh "pipeline" $END "$ands" >> $2;

#BUILD SYSTEM NODE
#echo "module System" >> $2;
#echo "    sys_broken: bool init false;" >> $2;
#echo "    [fail_pipeline??] -> (sys_broken'=true);" >> $2;
#echo "    [repair_pipeline??] -> (sys_broken'=false);" >> $2;
#echo "endmodule" >> $2;

#BUILD PROPERTIES
echo -e "\n\nproperties\n  S( " >> $5;
for i in $(seq 0 $((${END}-1))); do
    clause="    ("
    for j in $(seq 1 ${2}); do
        clause=$clause" broken_pipe$(($i+$j))>0 &"
    done
    clause=$clause" true ) |"
    echo "$clause" >> $5;
done
echo -e "    false )\nendproperties" >> $5;

