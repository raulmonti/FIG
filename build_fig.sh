#!/usr/bin/env bash

##==============================================================================
##
##  build_fig.sh
##	
##	Copyleft 2018-
##	Authors:
##  * Carlos E. Budde <cbudde@famaf.unc.edu.ar> (Universidad Nacional de Córdoba)
##
##------------------------------------------------------------------------------
##
##  This file is part of FIG.
##
##  The Finite Improbability Generator (FIG) project is free software;
##  you can redistribute it and/or modify it under the terms of the GNU
##  General Public License as published by the Free Software Foundation;
##  either version 3 of the License, or (at your option) any later version.
##
##  FIG is distributed in the hope that it will be useful,
##	but WITHOUT ANY WARRANTY; without even the implied warranty of
##	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
##	GNU General Public License for more details.
##	
##	You should have received a copy of the GNU General Public License
##	along with FIG; if not, write to the Free Software Foundation,
##	Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
##
##==============================================================================

set -e

# Use enhanced getopt to parse CLI options
# See https://stackoverflow.com/a/29754866
set -o errexit -o pipefail -o noclobber -o nounset;
! getopt --test > /dev/null;
if [[ ${PIPESTATUS[0]} -ne 4 ]]; then
	echo "$0: \"getopt --test\" failed in this environment; aborting";
	exit 1
fi
OPTIONS=p:m:c:v
LONGOPTS=project:,mode:,compiler:,verbose
# Parse CLI with enhanced getopt
! PARSED=$(getopt --options=$OPTIONS --longoptions=$LONGOPTS --name "$0" -- "$@")
if [[ ${PIPESTATUS[0]} -ne 0 ]]; then
	exit 2;  # getopt has complained about wrong arguments to stdout
else
	p="main" m="release" c="clang" v="no";  # default values
	eval set -- "$PARSED";  # extra arguments (outside the OPTIONS)
fi
# Interpret options
while true; do
	case "$1" in
	-p|--project)
		p="$2";
		if [[ "$p" != "main" && "$p" != "tests" ]]; then
			echo "[ERROR] Unrecognized project type \"$p\"";
			echo "        Valid arguments for -p are \"main\" and \"tests\"";
			exit 3;
		fi
		shift 2;
		;;
	-m|--mode)
		m="$2";
		if [[ "$m" != "release" && "$m" != "debug" ]]; then
			echo "[ERROR] Unrecognized build mode \"$m\"";
			echo "        Valid arguments for -m are \"release\" and \"debug\"";
			exit 3;
		fi
		shift 2;
		;;
	-c|--compiler)
		c="$2";
		if [[ "$c" != "clang" && "$c" != "gcc" ]]; then
			echo "[ERROR] Unrecognized compiler \"$c\"";
			echo "        Valid arguments for -c are \"clang\" and \"gcc\"";
			exit 3;
		fi
		shift 2;
		;;
	-v|--verbose)
		v="y";
		shift;
		;;
	--)
		shift;
		break;
		;;
	*)
		echo "[ERROR] Programming error (wrong getopt version?)";
		exit 3;
		;;
	esac
done
# Handle non-option arguments
if [[ $# -ge 1 ]]; then
	echo "$0: You passed your made-up option \"$@\"; thank you!";
	exit 3;
fi
[[ ! "$v" =~ "y" ]] && (echo "Will build $p in $m mode with $c compiler"; echo);


# Project type
if [[ "$p" == "main" ]]; then
	[[ "$v" =~ "y" ]] && (echo "Building main project");
	BUILD="fig";
	DIR=".";
elif [[ "$p" = "tests" ]]; then
	[[ "$v" =~ "y" ]] && (echo "Building tests suite");
	BUILD="test";
	DIR="tests";
else
	echo "[ERROR] Unrecognized project type \"$p\"";
	echo "        Valid arguments for -p are \"main\" and \"tests\"";
	exit 4;
fi


# Build mode
if [[ "$m" == "release" ]]; then
	[[ "$v" =~ "y" ]] && (echo "Building in release mode";);
	RELEASE_BUILD=true;
elif [ "$m" = "debug" ]; then
	[[ "$v" =~ "y" ]] && (echo "Building in debug mode";);
	RELEASE_BUILD=false;
else
	echo "[ERROR] Unrecognized build mode \"$m\"";
	echo "        Valid arguments for -m are \"release\" and \"debug\"";
	exit 4;
fi


# C compiler
CC="";
CXX="";
if [[ "$c" == "clang" ]] && [ ! "`which clang 2>/dev/null`" ]; then
	echo "[WARNING] No Clang compiler found in this environment";
	echo "          Switching to gcc";
elif [[ "$c" == "clang" ]]; then
	# We need version 3.7 or later
	CLANG_VERSION_MAJOR=$(clang --version | grep -o "[0-9]\.[0-9.]*" | head -1);
	CLANG_VERSION_MINOR=$(echo $CLANG_VERSION_MAJOR | cut -d"." -f 2);
	CLANG_VERSION_MAJOR=$(echo $CLANG_VERSION_MAJOR | cut -d"." -f 1);
	if [ $CLANG_VERSION_MAJOR -ge 4 ] ||
		([ $CLANG_VERSION_MAJOR -ge 3 ] && [ $CLANG_VERSION_MINOR -ge 7 ])
	then
		CC=`which clang`;
		CXX=`which clang++`;
	else
		echo "[WARNING] Clang is lower than version 3.7 in this environment";
		echo "          Switching to gcc";
	fi
fi
if [ -z "$CC" ] && [ "`which gcc`" ]; then
	# Any c++11 compatible version is fine (that's checked in cmake)
	CC=`which gcc`;
	CXX=`which g++`;
fi
if [[ $HOSTNAME == "mendieta" ]]  # Mendieta requires special treatment
then
	module load gcc;
	CC=`which gcc`;
	CXX=`which g++`;
	module load bison;  # for GNU Bison 3.0.4
fi
if [ -z "$CC" ]; then
	echo "[ERROR] No compatible C compiler was found, aborting.";
	exit 4;
fi
[[ "$v" =~ "y" ]] && (echo "Building with C compiler \"$CC\""; echo);
COMPILER="`basename $CC`";



# Check there's a CMake build file in the corresponding directory
if [ ! -f ${DIR}/CMakeLists.txt ]; then
	if [ -z $DIR ]; then DIR="current directory"; fi;
	echo "[ERROR] Couldn't find CMakeLists.txt file in $DIR, aborting.";
	exit 4;
else
	CWD=$PWD;
	CMAKE_DIR=$PWD/$DIR;
fi

# Cmake build options, see CMakeLists.txt
BUILD_OPTS="";
#BUILD_OPTS="$BUILD_OPTS -DBUILTIN_RNG=ON";
#BUILD_OPTS="$BUILD_OPTS -DPROFILING=ON";
if ! $RELEASE_BUILD ; then
	MODE="debug";
else
	MODE="release";
	BUILD_OPTS="$BUILD_OPTS -DRELEASE=ON";
fi

# Set CMake's build subdir (create a neat build tree)
BUILD_BASE=./build;
BUILD_DIR=$BUILD_BASE/${BUILD}_files_${MODE}_${COMPILER};
NJOBS=$(2>/dev/null bc <<< "2*`nproc --all`");
if [ -z "$NJOBS" ]; then
	NJOBS=2;
fi
mkdir -p $BUILD_DIR && cd $BUILD_DIR;
CC=$CC CXX=$CXX cmake $CMAKE_DIR $BUILD_OPTS && make -j$NJOBS && \
cd $CWD;

# Symlink main executable in current dir and in BUILD_BASE
FIND_CMD=`echo -maxdepth 2 -type f -executable -name $BUILD -print -quit`;
ln -rsf `find $BUILD_DIR $FIND_CMD` -t .;
ln -rsf ./$BUILD -t $BUILD_BASE;
[[ "$v" =~ "y" ]] && (/bin/echo -e "\n  Project built in $BUILD_DIR\n");

# Clean and leave, in that order please
unset -v FIND_CMD;
unset -v NJOBS;
unset -v COMPILER
unset -v BUILD_OPTS;
unset -v BUILD;
unset -v BUILD_BASE;
unset -v BUILD_DIR;
unset -v CMAKE_DIR;
unset -v CWD;
unset -v DIR;

exit 0;

