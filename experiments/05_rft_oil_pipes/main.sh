
# $1 : number of pipes
# $2 : output file name for model
# $3 : output file name for props
# TODO enable seting consecutive broken pipes limite (now 4)


echo "" > ${2} 
echo "" > ${3}

#BUILD PIPES
for i in $(seq 1 ${1}); do bash be.sh pipe$i >> $2 ; done

#BUILD ANDS
END=$((${1}-3))
for i in $(seq 1 $END); do 
    bash and.sh and$i 6 "pipe$((${i})) pipe$((${i}+1)) pipe$((${i}+2)) pipe$((${i}+3))"  >> $2 ;
done

#BUILD THE REPAIRMAN
bes=""
for i in $(seq 1 ${1}); do
    bes=${bes}" pipe$i"
done
bash repman.sh "pepe" ${1} "$bes" >> $2;

#BUILD THE OR TOP NODE
ands=""
for i in $(seq 1 $END); do
    ands=${ands}" and$i"
done
bash or.sh "pipeline" $END "$ands" >> $2;

#BUILD SYSTEM NODE
echo "module System" >> $2;
echo "    sys_broken: bool init false;" >> $2;
echo "    [fail_pipeline??] -> (sys_broken'=true);" >> $2;
echo "    [repair_pipeline??] -> (sys_broken'=false);" >> $2;
echo "endmodule" >> $2;



#BUILD PROPS FILE
echo "S(sys_broken==true)" >> $3;
