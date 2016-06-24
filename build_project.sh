#!/bin/sh

##==============================================================================
##
##  build_project.sh
##	
##	Copyleft 2015-
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

# Choose compiler (prefer clang over gcc)
if [ "`which clang`" ]
then
	# We need version 3.7 or later
	CLANG_VERSION_MAJOR=$(clang --version | grep -o "[0-9]\.[0-9.]*" | head -1)
	CLANG_VERSION_MINOR=$(echo $CLANG_VERSION_MAJOR | cut -d"." -f 2)
	CLANG_VERSION_MAJOR=$(echo $CLANG_VERSION_MAJOR | cut -d"." -f 1)
	if [ $CLANG_VERSION_MAJOR -ge 3 ] && [ $CLANG_VERSION_MINOR -ge 7 ]
	then
		CCOMP=clang
	fi
fi
if [ -z "$CCOMP" ] && [ "`which gcc`" ]
then
	# Any c++11 compatible version is fine (that's checked in cmake)
	CCOMP=gcc
fi
if [ -z "$CCOMP" ]
then
	echo "[ERROR] No compatible clang or gcc version was found, aborting."
	exit 1
fi

# Recognize requested build
if [ $# -eq 0 ]
then
	echo; echo "Called with no arguments, building main project"; echo
	DIR=""
elif [ $# -eq 1 ]
then
	if [ "$1" = "main" ]
	then
		echo; echo "Building main project"; echo
		DIR=""
	elif [ "$1" = "test_tads" ]
	then
		echo; echo "Building tests for TADs instantiation"; echo
		DIR="tests/tads/"
	elif [ "$1" = "test_parser" ]
	then
		echo; echo "Building tests for parser"; echo
		DIR="tests/parser/"
	elif [ "$1" = "test_psat" ]
	then
		echo; echo "Building tests for property satisfiability"; echo
		DIR="tests/PropertySat/"
	elif [ "$1" = "test_sims" ]
	then
		echo; echo "Building tests for simulations"; echo
		DIR="tests/simulations/"
	elif [ "$1" = "test_est" ]
	then
		echo; echo "Building tests for estimations"; echo
		DIR="tests/estimations/"
	else
		echo "[ERROR] Unrecognized build option \"$1\""
		echo "        Available builds are: main"
		echo "                              test_tads"
		echo "                              test_parser"
		echo "                              test_psat"
		echo "                              test_sims"
		echo "                              test_est"
		exit 1
	fi
else
	echo "[ERROR] call with at most one argument specifying the build type"
	echo "        Available builds are: main"
	echo "                              test_tads"
	echo "                              test_parser"
	echo "                              test_psat"
	echo "                              test_sims"
	echo "                              test_est"
	exit 1
fi

sleep 1

# Check there's a CMake build file in the corresponding directory
if [ ! -f ${DIR}CMakeLists.txt ]
then
	if [ -z $DIR ]; then DIR="current directory"; fi
	echo "[ERROR] Couldn't find CMakeLists.txt file in $DIR, aborting."
	exit 1
else
	CWD=$PWD
	CMAKE_DIR=$PWD/$DIR
fi

# Locate CMake's build sub-directory
BUILD_DIR=$(grep -i "SET[[:blank:]]*([[:blank:]]*PROJECT_BINARY_DIR" \
                    ${DIR}CMakeLists.txt)
if [ -z "$BUILD_DIR" ]
then
	echo "[ERROR] Couldn't find CMake's build directory, aborting."
	exit 1
else
	BUILD_DIR=$(echo $BUILD_DIR | gawk 'BEGIN {FS="/"};{print $NF}')
	if [ -z "$BUILD_DIR" ]
	then
		echo "[ERROR] Only SUB-directories are supported for builds, aborting."
		exit 1
	else
		BUILD_DIR=$(echo $BUILD_DIR | grep -o "[0-9a-zA-Z/_\-\.]" | tr '\n' '\0')
	fi
fi

# Configure and build from inside BUILD_DIR
if [ ! -d $BUILD_DIR ]; then mkdir $BUILD_DIR; fi
cd $BUILD_DIR
OPTS="$OPTS -DRELEASE=ON"      # Cmake build options, see CMakeLists.txt
OPTS="$OPTS -DBUILTIN_RNG=ON"  # Cmake build options, see CMakeLists.txt
#CC=$CCOMP CXX=${CCOMP%cc}++ cmake $CMAKE_DIR $OPTS && make && \
CC=gcc CXX=g++ cmake $CMAKE_DIR $OPTS && make && \
/bin/echo -e "\n  Project built in $BUILD_DIR\n"
cd $CWD

# Symlink main executable to current dir
EXE=`find $BUILD_DIR -type f -executable -name "fig" || \
	 find $BUILD_DIR -type f -executable -name "test_*"`;
if [ -f "fig" ]; then rm fig; fi; ln -s $EXE fig

unset EXE
unset CWD
unset OPTS
unset DIR
unset CMAKE_DIR
unset BUILD_DIR

exit 0

