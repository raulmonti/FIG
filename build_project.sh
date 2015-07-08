#!/bin/sh

set -e

# Check dependencies
if [ ! -f CMakeLists.txt ]
then
	echo "[ERROR] Couldn't find CMakeLists.txt file in current dir, aborting."
	exit 1
else
	CWD=$PWD
fi

# Locate CMake's build sub-directory
DIR=$(grep -i "SET[[:blank:]]*([[:blank:]]*PROJECT_BINARY_DIR" CMakeLists.txt)
if [ -z "$DIR" ]
then
	echo "[ERROR] Couldn't find CMake's build directory, aborting."
	exit 1
else
	DIR=$(echo $DIR | cut -d"/" -f 2)
	if [ -z "$DIR" ]
	then
		echo "[ERROR] Only SUB-directories are supported for builds, aborting."
		exit 1
	else
		DIR=$(echo $DIR | grep -o "[0-9a-zA-Z/_\-\.]" | tr '\n' '\0')
	fi
fi

# Choose compiler
if [ "`which clang`" ]; then CCOMP=clang
elif [ "`which gcc`" ]; then CCOMP=gcc
else
	echo "[ERROR] Nor Clang neither GNU C/C++ compiler was found, aborting."
	exit 1
fi
CCOMP=gcc  # FIXME Override for now

# Configure and build from inside DIR
if [ ! -d $DIR ]; then mkdir $DIR; fi
cd $DIR && CC=$CCOMP CXX=${CCOMP%cc}++ cmake $CWD && make
echo "\n  Project built in $PWD\n"
cd $CWD

exit 0

