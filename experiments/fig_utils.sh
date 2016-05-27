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


# Extract single time value from results dir,
# for specified importance function, experiment and splitting
extract_time() {
	# Check arguments
	if [ $# -lt 4 ]; then
		echo "[ERROR] Need dir name, ifun name, experiment and splitting"
		return 1
	fi
	local DIR=$1
	local IFUN=$2
	local EXP=$3
	local SPLIT=$4
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
	local TIME=$(grep --after=10 "splitting $SPLIT" $FILE | grep "time")
	if [ -z "${TIME}" ]; then
		echo "-"    # No estimation :(
	else
		TIME=$(echo $TIME | cut -d':' -f 2 | cut -d' ' -f 2 | cut -d\. -f 1)
		echo $TIME  # Estimation time in seconds
	fi
	return 0
}


# Extract single estimate from results dir,
# for specified importance function, experiment, splitting and confidence co.
extract_estimate() {
	# Check arguments
	if [ $# -lt 5 ]; then
		echo "[ERROR] Need dir name, ifun name, experiment, splitting and conf. co."
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
	# Retrieve and print requested estimate (or '-' if not found)
	local EST=$(grep --after=8 "splitting $SPLIT" $FILE | \
	            grep --after=7 "Confidence level: $CONF")
	if [ -z "${EST}" ]; then
		echo "-"      # No estimate   :(
	elif [[ "$EST" =~ "Estimation time:" ]]; then
		EST=$(echo $EST | grep -o "estimate:[[:space:]][1-9]\.[0-9]*e\-[0-9]*")
		EST=$(echo $EST | awk '{ print $2 }')
		echo "$EST"   # Good estimate :)
	else
		EST=$(echo $EST | grep -o "estimate:[[:space:]][1-9]\.[0-9]*e\-[0-9]*")
		EST=$(echo $EST | awk '{ print $2 }')
		echo "*$EST"  # Estimate w/TO :|
	fi
	return 0
}


# Print format "IFUN SPLIT VALUES[*]" for up to six values
print_time_line() { printf "%5s %3s %6s %6s %6s %6s %6s %6s\n" "${@}"; }
print_estimate_line() { printf "%5s %3s %10s %10s %10s %10s %10s %10s\n" "${@}"; }


# Extract results and print them formatted as a table into stdout
#   PARAM_1: "est" for estimates table and "time" for times table
#   PARAM_2: directory path where results are stored
#   PARAM_3: array with experiments names, or some way to identify them
#   PARAM_4: array with ifuns names
#   PARAM_5: array with splittings used
#   PARAM_6: confidence coefficient requested
#   PARAM_7: precision requested
build_table() {
	# Check arguments
	if [ $# -ne 7 ]; then
		echo "[ERROR] Bad calling arguments"
		return 1
	else
		local RESULTS="$2"
		local EXPERIMENTS=("${!3}")
		local IMPFUNS=("${!4}")
		local SPLITS=("${!5}")
		local CONF="$6"
		local PREC="$7"
		# Format confidence level and relative precision
		if [[ ! "$CONF" =~ "." ]]; then
			echo "[ERROR] Requires confidence coefficient ∈ (0.0 , 1.0)"
			return 1
		elif [[ ! "$PREC" =~ "." ]]; then
			echo "[ERROR] Requires relative precision"
			return 1
		else
			CONF=$(echo "$CONF*100" | bc -l)
			CONF=${CONF%%.*}"%"
			PREC=$(echo "$PREC*100" | bc -l)
			PREC=${PREC%%.*}"%"
		fi
		# Check whether be build for times or estimates
		if [[ "$1" == "est" ]]; then
			local PRINT=`echo "print_estimate_line"`
			local EXTRACT=`echo "extract_estimate"`
			echo "Estimates for $CONF confidence and $PREC precision"
		elif [[ $1 == "time" ]]; then
			local PRINT=`echo "print_time_line"`
			local EXTRACT=`echo "extract_time"`
			echo "Times (in sec) for $CONF confidence and $PREC precision"
		else
			echo "[ERROR] First argument must be either \"est\" or \"time\"";
			return 1
		fi
	fi
	$PRINT "" "" "${EXPERIMENTS[@]}"
	# For each importance function
	for IF in "${IMPFUNS[@]}"; do
		local TIMES=()
		local idx=0
		# Standard Monte Carlo? No splitting
		if [[ $IF == "MC" ]]; then
			idx=0
			for EXP in "${EXPERIMENTS[@]}"; do
				TIMES[$idx]=$($EXTRACT $RESULTS "$IF" "$EXP" "1" "$CONF")
				idx=$((idx+1))
			done
			$PRINT "$IF" "" "${TIMES[@]}"
			continue;
		fi
		# All the rest: print for all splittings
		for S in "${SPLITS[@]}"; do
			idx=0;
			for EXP in "${EXPERIMENTS[@]}"; do
				TIMES[$idx]=$($EXTRACT $RESULTS "$IF" "$EXP" "$S" "$CONF")
				idx=$((idx+1))
			done
			$PRINT "$IF" "$S" "${TIMES[@]}"
			continue;
		done
	done
	return 0
}

return 0

