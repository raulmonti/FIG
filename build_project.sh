#!/bin/bash

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
unset -v CC CXX
if [ "`which clang 2>/dev/null`" ]
then
	# We need version 3.7 or later
	CLANG_VERSION_MAJOR=$(clang --version | grep -o "[0-9]\.[0-9.]*" | head -1)
	CLANG_VERSION_MINOR=$(echo $CLANG_VERSION_MAJOR | cut -d"." -f 2)
	CLANG_VERSION_MAJOR=$(echo $CLANG_VERSION_MAJOR | cut -d"." -f 1)
	if [ $CLANG_VERSION_MAJOR -ge 4 ] ||
		([ $CLANG_VERSION_MAJOR -ge 3 ] && [ $CLANG_VERSION_MINOR -ge 7 ])
	then
		CC=clang
		CXX=clang++
	fi
fi
if [ -z "$CC" ] && [ "`which gcc`" ]
then
	# Any c++11 compatible version is fine (that's checked in cmake)
	CC=`which gcc`
	CXX=`which g++`
fi
if [[ $HOSTNAME == "mendieta" ]]  # Mendieta requires special treatment
then
	module load gcc
	CC=`which gcc`
	CXX=`which g++`
#	module unload gcc
#	module unload intel
#	module load xeonphi/2016
#	CC=`which icc`
#	CXX=$CC
	module load bison  # for GNU Bison 3.0.4
fi
if [ -z "$CC" ]
then
	echo "[ERROR] No compatible C compiler was found, aborting."
	exit 1
fi

# Recognize requested build
if [ $# -eq 0 ]
then
	echo; echo "Called with no arguments, building main project"; echo
	DIR="./"
elif [ $# -eq 1 ]
then
	if [ "$1" = "main" ]
	then
		echo; echo "Building main project"; echo
		DIR="./"
	elif [ "$1" = "tests" ]
	then
		echo; echo "Building test suite"; echo
		DIR="tests/"
	else
		echo "[ERROR] Unrecognized build option \"$1\""
		echo "        Available builds are \"main\" and \"tests\""
		exit 1
	fi
else
	echo "[ERROR] call with at most one argument specifying the build type"
	echo "        Available builds are \"main\" and \"tests\""
	exit 1
fi

sleep .1  # FIXME: what was this here for?

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
		BUILD_DIR=$(echo $BUILD_DIR | grep -o "[0-9a-zA-Z/_\-\.]" | tr -d '\n')
	fi
fi

# Configure and build from inside BUILD_DIR
if [ ! -d $BUILD_DIR ]; then mkdir $BUILD_DIR; fi
cd $BUILD_DIR
OPTS="$OPTS -DRELEASE=ON"      # Cmake build options, see CMakeLists.txt
OPTS="$OPTS -DBUILTIN_RNG=ON"  # Cmake build options, see CMakeLists.txt
#OPTS="$OPTS -DPROFILING=ON"    # Cmake build options, see CMakeLists.txt
NJOBS=$(2>/dev/null bc <<< "2*`nproc --all`")
if [ -z "$NJOBS" ]; then NJOBS=2; fi
CC=$CC CXX=$CXX cmake $CMAKE_DIR $OPTS && make -j$NJOBS && \
#CC=gcc CXX=g++ cmake $CMAKE_DIR $OPTS && make -j$NJOBS && \
/bin/echo -e "\n  Project built in $BUILD_DIR\n"
cd $CWD

# Symlink main executable to current dir
EXE=`find $BUILD_DIR -type f -executable -name "fig" || \
	 find $BUILD_DIR -type f -executable -name "test_*"`;
ln -sf $EXE fig

unset -v EXE
unset -v NJOBS
unset -v OPTS
unset -v BUILD_DIR
unset -v CMAKE_DIR
unset -v CWD
unset -v DIR
unset -v CC
unset -v CXX

exit 0

