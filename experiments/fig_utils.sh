#!/bin/bash
#
# Author:  Carlos E. Budde
# Date:    22.03.2016
# License: GPLv3
#
# NOTE: this script should be sourced from another script
# Cool site: http://wiki.bash-hackers.org
#


# Following assumes this script file is one level deeper than the project base
BASE_DIR=`readlink -f "$(dirname ${BASH_SOURCE[0]})/.."`
if [ ! -d $BASE_DIR ]
then
	echo "[ERROR] Couldn't find project's base directory"
	return 1
fi


# Check argument is a regular file
is_file() {
	if [ ! -f "$1" ]
	then
		echo "[ERROR] Couldn't find file \"$1\"" 1>&2
		exit 1
	else
		echo "File \"$1\" found"
	fi
}


# Build main FIG project and link binary into specified (absolute) path
build_fig() {
	# Check arguments
	if [ $# -lt 1 ]
	then
		echo "[ERROR] Must provide absolute path to link FIG's binary into"
		return 1
	elif [ ! -d $1 ]
	then
		echo "[ERROR] \"$1\" isn't a valid path"
		return 1
	elif [[ "$1" != /* ]]
	then
		echo "[ERROR] An *absolute* path for linking must be given"
		return 1
	fi
	is_file "${BASE_DIR}/CMakeLists.txt"   >/dev/null
	is_file "${BASE_DIR}/build_project.sh" >/dev/null
	# Build if need be
	CWD=$PWD
	cd $BASE_DIR
	if [ ! -d bin ]
	then
		./build_project.sh main
	elif [ ! -f bin/fig/fig ]
	then
		if [ -d OLD_bin ]; then rm -rf OLD_bin; fi; mv bin OLD_bin
		./build_project.sh main
	elif `bin/fig/fig --help &> /dev/null` [ $? -ne 0 ]
	then
		rm -rf bin
		./build_project.sh main
	fi
	cd $CWD
	if [ ! -f $BASE_DIR/bin/fig/fig ]
	then
		echo "[ERROR] Couldn't build FIG project."
		return 1
	fi
	# Link where requested
	if [ -f $1/fig ]; then rm $1/fig; fi
	ln -sf $BASE_DIR/bin/fig/fig $1/fig
}


# Copy some SA model file from the FIG project into specified (absolute) path
copy_model_file() {
	# Check arguments
	if [ $# -lt 1 ]
	then
		echo "[ERROR] Must provide the name of the model file to copy"
		return 1
	elif [ $# -lt 2 ]
	then
		echo "[ERROR] Must provide a path to copy the model into"
		return 1
	elif [ ! -d $2 ]
	then
		echo "[ERROR] \"$2\" isn't a valid directory"
		return 1
	elif [[ "$2" != /* ]]
	then
		echo "[ERROR] An *absolute* path to copy the model into must be given"
		return 1
	fi
	# Look for the model file
	local MODELS_DIR="$BASE_DIR/models"
	if [ ! -d $MODELS_DIR ]
	then
		echo "[ERROR] Couldn't find models directory \"$MODELS_DIR\""
		return 1
	elif [ ! -f $MODELS_DIR/$1 ]
	then
		echo "[ERROR] Couldn't find model file \"$1\" in dir \"$MODELS_DIR\""
		return 1
	fi
	# Copy the file into the requested path
	cp $MODELS_DIR/$1 $2
}


# Poll until less than MAXJOBSN jobs called 'fig' are running
# Optionally takes part of the invocation command as single argument
# for thinner filtering
poll_till_free() {
	if [ -z "$MAXJOBSN" ]
	then
		local JOBSBOUND=`nproc --all`
	else
		local JOBSBOUND=$MAXJOBSN
	fi
	# do-while syntax sugar for bash
	while
#		local RUNNING=`pgrep -u "$(whoami)" fig | wc -l`
		local RUNNING=`ps -fC "fig" | grep "$1" | wc -l`
		[ $RUNNING -ge $JOBSBOUND ]
	do
		sleep 10s
	done
}


# Print passed number of seconds in the format dd-hh:mm:ss
# where 'dd' are days, 'hh' are hours, 'mm' are minutes and 'ss' are seconds
# e.g. argument 3723 (seconds) is printed as "0-01:02:03"
# Note: bash, ksh and zsh define the $SECONDS variable that counts the
#       number of seconds since the shell was started
# See: http://unix.stackexchange.com/a/52318
format_seconds() {
	local S_DAY=86400  # seconds in a day
	local S_HOUR=3600  # seconds in an hour
	local S_MINUTE=60  # seconds in a minute
	# Check arguments
	if [ $# -ne 1 ]
	then
		echo "[ERROR] Single integer argument (seconds) required"
		return 1
	else
		# Format seconds and print
		local DAYS=$(($1 / S_DAY))
		local HOURS=$((($1 % S_DAY) / S_HOUR))
		local MINUTES=$((($1 % S_HOUR) / S_MINUTE))
		printf "%d-%02d:%02d:%02d" $DAYS $HOURS $MINUTES $(($1%S_MINUTE))
		return 0
	fi
}


# Print number of seconds corresponding to passed duration,
# e.g. argument "2h" yields "7200" and "15m" yields "900".
# The argument's single character suffix, which can be any from the set [smhd],
# specifies whether the numerical part represents seconds (s), minutes (m),
# hours (h), or days (d). By default no suffix is interpreted as seconds.
compute_seconds() {
	if [ $# -ne 1 ]; then
		echo "[ERROR] Argument required (duration with suffix)"; return 1
	elif [[ $1 =~ ^[0-9]+d$ ]]; then
		echo $(echo "scale=0; ${1%d}*86400" | bc)  # days
	elif [[ $1 =~ ^[0-9]+h$ ]]; then
		echo $(echo "scale=0; ${1%h}*3600" | bc)   # hours
	elif [[ $1 =~ ^[0-9]+m$ ]]; then
		echo $(echo "scale=0; ${1%m}*60" | bc)     # minutes
	elif [[ $1 =~ ^[0-9]+s$ ]] || [[ $1 =~ ^[0-9]+$ ]]; then
		echo ${1%s}
	else
		echo "[ERROR] Bad argument: \"$1\""; return 1
	fi
	return 0
}


# Extract single time value from results dir, for specified importance function,
# experiment, splitting and confidence level
extract_time() {
	# Check arguments
	if [ $# -ne 5 ]; then
		echo "[ERROR] Need dir name, ifun name, experiment, splitting and confidence level"
		return 1
	elif [[ ! $5 =~ ^[0-9]+%$ ]]; then
		echo "[ERROR] Need confidence level (with '%' suffix) as fifth argument"
		return 1
	fi
	local DIR=$1
	local IFUN=$2
	local EXP=$3
	local SPLIT=$4
	local CONF=$5
	# Find results file
	local FILE=`find "$DIR" -name "*${IFUN}*${EXP}*.out"`
	if [ -z "${FILE}" ]; then
		FILE=`find "$DIR" -name "*${EXP}*${IFUN}*.out"`
		if [ -z "${FILE}" ]; then
			echo "[ERROR] Didn't find results for \"${IFUN}\" and \"${EXP}\""
			return 1
		fi
	fi
	# Retrieve and print requested time value (or '-' if not found)
	local TIME=$(awk "/splitting $SPLIT/{f=1;next} /splitting/{f=0} f" $FILE \
	            | grep --after=9 "Confidence level: $CONF" | grep "time")
	if [ -z "${TIME}" ]; then
		echo "-"    # No valid estimation found   :(
	else
		TIME=$(echo $TIME | cut -d':' -f 2 | cut -d' ' -f 2 | cut -d\. -f 1)
		echo $TIME  # Estimation time in seconds  :)
	fi
	return 0
}


# TODO erase deprecated function
## # Extract single estimate from results dir, for specified importance function,
## # experiment, splitting, and confidence level or time bound.
## extract_estimate() {
## 	# Check arguments
## 	if [ $# -ne 5 ]; then
## 		echo "[ERROR] Need dir name, ifun name, experiment, splitting, and confidence level or time bound"
## 		return 1
## 	elif [[ ! $5 =~ ^[0-9]+%$ ]] && [[ ! $5 =~ ^[0-9]+[smhd]?$ ]]; then
## 		echo "[ERROR] Need confidence level or time bound as fifth argument"
## 		return 1
## 	fi
## 	local DIR=$1
## 	local IFUN=$2
## 	local EXP=$3
## 	local SPLIT=$4
## 	# Find file with results
## 	local FILE=`find "$DIR" -name "*${IFUN}*${EXP}*.out"`
## 	if [ -z "${FILE}" ]; then
## 		FILE=`find "$DIR" -name "*${EXP}*${IFUN}*.out"`
## 		if [ -z "${FILE}" ]; then
## 			echo "[ERROR] Didn't find results for \"${IFUN}\" and \"${EXP}\""
## 			return 1
## 		fi
## 	fi
## 	# Retrieve and print requested estimate (or '-' if not found)
## 	if [[ $5 =~ ^[0-9]+%$ ]]; then
## 		local CONF=$5
## 		local EST=$(awk "/splitting $SPLIT/{f=1;next} /splitting/{f=0} f" \
## 		            $FILE | grep --after=7 "Confidence level: $CONF")
## 	elif [[ $5 =~ ^[0-9]+[smhd]?$ ]]; then
## 		local TIMEOUT=$(compute_seconds $5)
## 		local EST=$(awk "/splitting $SPLIT/{f=1;next} /splitting/{f=0} f" \
## 		            $FILE | grep --after=7 "Estimation timeout: $TIMEOUT s")
## 	fi
## 	if [ -z "${EST+x}" ]; then  # http://stackoverflow.com/a/13864829
## 		echo "-"      # No estimate   :(
## 	elif [[ "$EST" =~ "Estimation time:" ]] || [[ -n "${TIMEOUT}" ]]; then
## 		EST=$(echo $EST | grep -o "estimate:[[:space:]][1-9]\.[0-9]*e\-[0-9]*")
## 		EST=$(echo $EST | awk '{ print $2 }')
## 		echo "$EST"   # Good estimate :)
## 	else
## 		EST=$(echo $EST | grep -o "estimate:[[:space:]][1-9]\.[0-9]*e\-[0-9]*")
## 		EST=$(echo $EST | awk '{ print $2 }')
## 		echo "*$EST"  # Estimate w/TO :|
## 	fi
## 	return 0
## }


# Extract single value (estimate or precision) from results dir,
# for specified importance function, experiment, splitting
# and confidence level or time bound.
extract_value() {
	# Check arguments
	if [ $# -lt 6 ]; then
		echo "[ERROR] Bad args, need (est|prec), dir name, ifun name, experiment, splitting, and confidence level or time bound"
		return 1
	elif [[ ! $1 =~ ^(est|prec)$ ]]; then
		echo "[ERROR] First argument must be either \"est\" or \"prec\""
		return 1
	elif [[ ! $6 =~ ^[0-9]+%$ ]] && [[ ! $6 =~ ^[0-9]+[smhd]?$ ]]; then
		echo "[ERROR] Sixth argument must be a confidence level or time bound,"
		echo "        e.g. \"80%\" or \"2h\""
		return 1
	fi
	local DIR=$2
	local IFUN=$3
	local EXP=$4
	local SPLIT=$5
	# Find file with results
	local FILE=`find "$DIR" -name "*${IFUN}*${EXP}*.out"`
	if [ -z "${FILE}" ] || [ ! -f $FILE ]; then
		FILE=`find "$DIR" -name "*${EXP}*${IFUN}*.out"`
		if [ -z "${FILE}" ] || [ ! -f $FILE ]; then
			echo "[ERROR] Didn't find results for \"${IFUN}\" and \"${EXP}\""
			return 1
		fi
	fi
	# Retrieve requested value
	if [[ $6 =~ ^[0-9]+%$ ]]; then
		local CONFLVL=$6
		local VAL=$(awk "/splitting $SPLIT/{f=1;next} /splitting/{f=0} f" $FILE \
		           | grep --after=7 "Confidence level: $CONFLVL")
	elif [[ $6 =~ ^[0-9]+[smhd]?$ ]]; then
		local TBOUND=$(compute_seconds $6)
		local VAL=$(awk "/splitting $SPLIT/{f=1;next} /splitting/{f=0} f" $FILE \
		           | grep --after=15 "Estimation timeout: $TBOUND s")
	fi
	# Print value (or '-' if not found)
	if [ -z "${VAL}" ]; then
		echo "-"  # No value found  :(
		return 0
	elif [[ -n "${CONFLVL}" ]] && [[ ! "$VAL" =~ "Estimation time:" ]]; then
		echo -n "*"  # Simulation timedout  :|
	fi
	if [[ $1 == "est" ]]; then
		local VMATCH="estimate:[[:space:]][1-9]\.[0-9]*e\-[0-9]*"
	elif [[ $1 == "prec" ]]; then
		local VMATCH="[Pp]recision:[[:space:]][1-9]\.[0-9]*e\-[0-9]*"
		if [ -n "${TBOUND}" ]; then
			VAL=$(echo $VAL | grep --after=1 "$7[[:space:]]confidence")
		fi
	fi
	VAL=$(echo $VAL | grep -o $VMATCH | awk '{ print $2 }')
	if [ -n "${VAL}" ]; then
		echo "$VAL"  # Valid value found!   :)
	else
		echo "-"     # No valid value found :(
	fi
	return 0
}


# Print format "IFUN SPLIT VALUES[*]" for up to six values
print_time_line() { printf "%5s %3s %6s %6s %6s %6s %6s %6s\n" "${@}"; }
print_value_line() { printf "%5s %3s %10s %10s %10s %10s %10s %10s\n" "${@}"; }


# Extract results and print them formatted as a table into stdout
#   PARAM_1: "est" for estimates, "prec" for precisions, "time" for times
#   PARAM_2: directory path where results are stored
#   PARAM_3: array with experiments names, or some way to identify them
#   PARAM_4: array with ifuns names
#   PARAM_5: array with splittings used
#   PARAM_6: confidence coefficient or time bound requested
#   PARAM_7: precision (for PARAM_6 "conf") or confidence (for PARAM_6 "time")
build_table() {
	# Check and format arguments
	if [ $# -ne 7 ]; then
		echo "[ERROR] Bad calling arguments"
		return 1
	else
		# Check which table to build
		if [[ "$1" == "est" ]]; then
			local HEADER_A="Estimates"
			local PRINT=`echo "print_value_line"`
			local EXTRACT=`echo "extract_value est"`
		elif [[ "$1" == "prec" ]]; then
			local HEADER_A="Precision values"
			local PRINT=`echo "print_value_line"`
			local EXTRACT=`echo "extract_value prec"`
		elif [[ $1 == "time" ]]; then
			local HEADER_A="Times (in sec)"
			local PRINT=`echo "print_time_line"`
			local EXTRACT=`echo "extract_time"`
		else
			echo "[ERROR] First param must be \"est\", \"prec\", or \"time\"";
			return 1
		fi
		local RESULTS="$2"
		local EXPERIMENTS=("${!3}")
		local IMPFUNS=("${!4}")
		local SPLITS=("${!5}")
		# Format confidence coefficient or time bound
		if [[ $6 =~ ^[0-9]*\.[0-9]*$ ]]; then
			local CONF=$(echo "$6*100" | bc -l)
			CONF=${CONF%%.*}"%"
			local HEADER_B="for $CONF confidence"
			local MATCH=$CONF
		elif [[ $6 =~ ^[0-9]+[smhd]?$ ]]; then
			local TIMEOUT=$(compute_seconds $6)"s"
			local HEADER_B="for $6 timeout"
			local MATCH=$TIMEOUT
		else
			echo "[ERROR] Requires confidence coefficient ∈ (0.0 , 1.0)" \
			     "or time bound > 0s"
			return 1
		fi
		# Format precision (for $6==conf) or confidence (for $6==time)
		if [[ $7 =~ ^[0-9]*\.[0-9]*$ ]]; then
			local PERCENT=$(echo "$7*100" | bc -l); PERCENT=${PERCENT%%.*}"%"
			if [ -n "${CONF}" ]; then
				local HEADER_C="and $PERCENT precision"
			elif [ -n "${TIMEOUT}" ] && [[ $1 == "prec" ]]; then
				MATCH+=" $PERCENT"
				local HEADER_C="and $PERCENT confidence"
			fi
		else
			echo "[ERROR] Bad last argument";
			return 1
		fi
	fi
	# Print header
	echo "$HEADER_A $HEADER_B $HEADER_C"
	$PRINT "" "" "${EXPERIMENTS[@]}"
	# For each importance function
	for IFUN in "${IMPFUNS[@]}"; do
		local VALUES=()
		local idx=0
		# Standard Monte Carlo? No splitting
		if [[ $IFUN == "MC" ]]; then
			idx=0
			for EXP in "${EXPERIMENTS[@]}"; do
				VALUES[$idx]=$($EXTRACT $RESULTS "$IFUN" "$EXP" "1" "$MATCH")
				idx=$((idx+1))
			done
			$PRINT "$IFUN" "" "${VALUES[@]}"
			continue;
		fi
		# All the rest: print for every splitting
		for S in "${SPLITS[@]}"; do
			idx=0;
			for EXP in "${EXPERIMENTS[@]}"; do
				VALUES[$idx]=$($EXTRACT $RESULTS "$IFUN" "$EXP" "$S" "$MATCH")
				idx=$((idx+1))
			done
			$PRINT "$IFUN" "$S" "${VALUES[@]}"
			continue;
		done
	done
	return 0
}

return 0

