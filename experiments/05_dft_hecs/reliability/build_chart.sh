LC_ALL_BKP=$LC_ALL;
LC_ALL=en_US.UTF-8;
#
# Author:  Carlos E. Budde
# Date:    11.12.2017
# License: GPLv3
#
#   Build summary chart for case study "HECS"
#   and print it to stdout.
#   NOTE: this script should be sourced from another script  XXX
#

# Check proper invocation
if [[ ! "$(type -t showe 2>&1)" =~ "function" ]]; then
	echo -e "[ERROR] Function \"showe()\" is not in the environment" >&2;
	echo -e "        This script must be *sourced* from main.sh" >&2;
	return 1;
elif [ -z $ISPLIT_CFG ] || [ ${#ISPLIT_CFG[@]} -lt 1 ] ; then
	showe "[ERROR] Couldn't find the list of I-SPLIT configurations";
	showe "        This script must be *sourced* from main.sh";
	return 1;
fi
TO="$1";
GE="$2";
TITLE="$3";
if [ $# -lt 2 ] || [ -z $TO ] || [ -z $GE ]; then
	showe "[ERROR] Must pass timeout and global-effort as parameters,";
	showe "        following the format used for the result file names.";
	return 1;
elif [ -z $TITLE ]; then
	TITLE="$EXPNAME";
fi

# Fetch row IDs from model file names
if [ -z $MODEL_LIST ] || [ ${#MODEL_LIST[@]} -lt 1 ] ; then
	showe "[ERROR] Couldn't find the list of model files";
	return 1;
fi
declare -a SYS_CFG;
for (( i=0 ; i < ${#MODEL_LIST[@]} ; i++ )); do
	CFG=`basename ${MODEL_LIST[i]}`;
	CFG=${CFG##*hecs_};
	CFG=${CFG%%_?p_*.sa};
	if [ -z $CFG ]; then
		showe "[ERROR] Model file #$i has an empty config? See MODEL_LIST[@]";
		return 1;
	elif [ ! -f "`ls $RESULTS/*$CFG*.out 2>&1 | head -n 1`" ]; then
		showe "[ERROR] No model file with config \"$CFG\" found in $RESULTS";
		return 1;
	else
		SYS_CFG[$i]=$CFG;
	fi
done

# Average rare event probabilities estimates
declare -a EST_AVG;
declare -a EST_STDEV;
for (( i=0 ; i < ${#SYS_CFG[@]} ; i++ )); do
	M2=0;
	COUNT=0;
	EST_AVG[$i]=0;
	EST_STDEV[$i]=NaN;
	CFG="${SYS_CFG[i]}";
	FILES=$RESULTS/*${CFG}*${TO}*${GE}*.out;
	# Incremental computation of mean and standard deviation,
	# omitting cases where there is no estimate:
	for EST in `gawk '/Computed estimate:/{print $4}' $FILES`; do
		if [[ ! $EST =~ "e-" ]] || [ ${EST##*e-} -le 0 ]; then
			continue;  # bad estimate value, skip
		fi
		EST=${EST/e-/*10^-};  # bc doesn't support scientific notation
		COUNT=$((COUNT+1));
		DELTA=$(bc -l <<< "($EST)-(${EST_AVG[i]})");
		EST_AVG[$i]=$(bc -l <<< "(${EST_AVG[i]})+($DELTA)/$COUNT");
		DELTA2=$(bc -l <<< "($EST)-(${EST_AVG[i]})");
		M2=$(bc -l <<< "($M2)+($DELTA)*($DELTA2)");
		if [ $COUNT -gt 1 ]; then
			EST_STDEV[$i]=$(bc -l <<< "($M2)/($COUNT-1)");
		fi
	done
done

# Build chart and print it to stdout
NUL="x";   # null/invalid value display
TIME_REGEXP="^[[:digit:]]+(\.[[:digit:]]+)?$";
show "#";
show "# $TITLE";
show "#";
show "# TO: ${TO}   GE: ${GE}";
show "#";
show -n "# Sys.cfg. & estimate ";
for IS_CFG in "${ISPLIT_CFG[@]}"; do
	printf "%16s" $IS_CFG;
done
show; # newline
for (( i=0 ; i < ${#SYS_CFG[@]} ; i++ )); do
	show "# ${SYS_CFG[i]}";
	printf "  %.2e  %.2e  " "${EST_AVG[i]}" "${EST_STDEV[i]}";
	for IS_CFG in ${ISPLIT_CFG[@]}; do
		FILE="${RESULTS}/*${SYS_CFG[i]}*${IS_CFG}*${TO}*";
		[[ $IS_CFG =~ flat ]] && FILE+=".out" || FILE+="${GE}*.out";
		TIME=$(gawk '/Estimation time:/{print $4}' $FILE 2>/dev/null);
		if [[ $TIME =~ $TIME_REGEXP ]]; then
			printf "%16.2f" "$TIME";  #    valid time value
		else
			printf "%16s" "$NUL";     # no valid time value
		fi
	done
	show;  # newline
done

LC_ALL=$LC_ALL_BKP;
return 0;
