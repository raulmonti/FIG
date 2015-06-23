#!/bin/sh

set -e

# Check dependencies
if [ ! -f CMakeLists.txt ]
then
	echo "Couldn't find CMakeLists.txt file in current dir, aborting."
	exit 1
else
	CWD=$PWD
fi

# Locate CMake's build sub-directory
DIR=$(grep -i "SET[[:blank:]]*([[:blank:]]*PROJECT_BINARY_DIR" CMakeLists.txt)
if [ -z "$DIR" ]
then
	echo "Couldn't find CMake's build directory"
	exit 1
else
	DIR=$(echo $DIR | cut -d"/" -f 2)
	if [ -z "$DIR" ]
	then
		echo "Only SUB-directories are supported for builds"
		exit 0
	else
		DIR=$(echo $DIR | grep -o "[0-9a-zA-Z/_\-\.]" | tr '\n' '\0')
	fi
fi

# Configure and build from inside DIR
if [ ! -d $DIR ]; then mkdir $DIR; fi
cd $DIR && cmake $CWD && make
echo "\n  Project built in $PWD\n"
cd $CWD

exit 0

