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

# Choose compiler (prefer clang over gcc)
if [ "`which clang`" ]
then
	# We need version 3.4 or later
	CLANG_VERSION_MAJOR=$(clang --version | grep -o "[0-9]\.[0-9.]*" | head -1)
	CLANG_VERSION_MINOR=$(echo $CLANG_VERSION_MAJOR | cut -d"." -f 2)
	CLANG_VERSION_MAJOR=$(echo $CLANG_VERSION_MAJOR | cut -d"." -f 1)
	if [ $CLANG_VERSION_MAJOR -ge 3 ] && [ $CLANG_VERSION_MINOR -ge 4 ]
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

# Configure and build from inside DIR
if [ ! -d $DIR ]; then mkdir $DIR; fi
cd $DIR && CC=$CCOMP CXX=${CCOMP%cc}++ cmake $CWD && make
echo "\n  Project built in $PWD\n"
cd $CWD

exit 0

