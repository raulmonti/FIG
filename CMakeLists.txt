##==============================================================================
##
##  CMakeLists.txt
##	
##	Copyleft 2015-
##	Authors:
##  * Carlos E. Budde <cbudde@famaf.unc.edu.ar> (Universidad Nacional de Córdoba)
##  * Raul E. Monti <raulmonti88@gmail.com> (Universidad Nacional de Córdoba)
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

CMAKE_MINIMUM_REQUIRED(VERSION 2.8.11)

PROJECT (fig)

SET(fig_VERSION_MAJOR 1)
SET(fig_VERSION_MINOR 0)

STRING(TIMESTAMP CURRENT_TIME "%H:%M:%S %d.%m.%Y")


 
###  Build meta options  ######################################################

OPTION(RELEASE     "Build release version of FIG"           OFF)
OPTION(BUILTIN_RNG "Use the PCG RNG provided with FIG"      OFF)
OPTION(USE_DTIME   "Double fp precision for time tracking"  OFF)

IF(RELEASE)
	MESSAGE("Building FIG in release mode")
	SET(fig_CURRENT_BUILD "${CURRENT_TIME} (release)")
ELSE()
	MESSAGE("Building FIG in debug mode")
	SET(fig_CURRENT_BUILD "${CURRENT_TIME} (debug)")
ENDIF()

 

###  Project directory structure  #############################################

# Default directories configuration
SET(SOURCES_DIR ${PROJECT_SOURCE_DIR}/src)
SET(HEADERS_DIR ${PROJECT_SOURCE_DIR}/include)
SET(LIBRARIES_DIR ${PROJECT_SOURCE_DIR}/lib)
SET(PROJECT_BINARY_DIR ${PROJECT_SOURCE_DIR}/bin)
SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/fig)

# Header file with CMake settings for the source code
CONFIGURE_FILE("${SOURCES_DIR}/FigConfig.h.in" "${HEADERS_DIR}/FigConfig.h")

# Allow subdirectories to append their files and dirs (https://goo.gl/xxIOAM)
SET(HEADERS_DIR ${HEADERS_DIR} CACHE INTERNAL "")
SET(ALL_SOURCES ${SOURCES_DIR} CACHE INTERNAL "")

# Visit subdirectories
ADD_SUBDIRECTORY(${PROJECT_SOURCE_DIR}/ext)
 
# Register all main include paths
INCLUDE_DIRECTORIES(${HEADERS_DIR})

# Register all main source files
AUX_SOURCE_DIRECTORY(${SOURCES_DIR} MAIN_SOURCE_FILES)
SET(ALL_SOURCES ${ALL_SOURCES} ${MAIN_SOURCE_FILES} CACHE INTERNAL "")

## # Libraries
## LINK_DIRECTORIES(${LIBRARIES_DIR})
##  Tip: use find_library() and find_package() instead



###  Preprocessor macros  #####################################################

# Build type: debug vs. release
IF(RELEASE)
	ADD_DEFINITIONS(-DNDEBUG)     # General No-Debug
	ADD_DEFINITIONS(-DNRANGECHK)  # Don't check ranges every f***ing time
ENDIF()

# RNG algorithm: C++ STL Mersenne-Twister vs. PCG default
IF(BUILTIN_RNG)
	MESSAGE("Using (alternative) RNG from the PCG family")
	ADD_DEFINITIONS(-DPCG_RNG)  # PCG family RNG (http://www.pcg-random.org/)
ELSE()
	MESSAGE("Using C++ STL's Mersenne-Twister RNG")
ENDIF()

# Time fp precision: single vs. double
IF(USE_DTIME)
	MESSAGE("Time tracked with double fp precision")
	ADD_DEFINITIONS(-DDOUBLE_TIME_PRECISION)
ELSE()
	MESSAGE("Time tracked with single fp precision")
ENDIF()

 

###  Compiler configuration  ##################################################
 
# If Clang was chosen check for compatible version
IF(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
	SET(CLANG_VERSION_REQUIRED 3.7)
	IF(CMAKE_CXX_COMPILER_VERSION VERSION_LESS CLANG_VERSION_REQUIRED)
		MESSAGE(FATAL_ERROR
			"Clang compiler version too old (${CMAKE_CXX_COMPILER_VERSION})"
			" -- ${CMAKE_C_COMPILER} ${CLANG_VERSION_REQUIRED} or greater "
			"is required.")
	ENDIF()
ENDIF()
MESSAGE("Using ${CMAKE_CXX_COMPILER_ID} compiler "
	    "version ${CMAKE_CXX_COMPILER_VERSION}")

# Demand support for c++11 standard
INCLUDE(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
IF(COMPILER_SUPPORTS_CXX11)
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
ELSE()
	MESSAGE(FATAL_ERROR "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support.")
ENDIF()


 
###  Compile flags  ###########################################################

# Build type: release vs. debug
IF(RELEASE)
	# As much automatic optimization as we can get
	IF(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
	ELSE()
		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 --fast-math -march=native")
		#SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 --fast-math -march=sandybridge")
	ENDIF()
ELSE()
	# Optimize without obfuscating the assembly so much
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O1")
	# Highly defensive compile flags
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")# -pedantic")
	# Add debug info (used by gdb and pals)
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
ENDIF()

# OpenMP
IF(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fopenmp=libgomp")
ELSE()
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fopenmp")
ENDIF()
## # Clang needs to be told where to look for OpenMP headers
## IF(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
##	FIND_PACKAGE(OpenMP REQUIRED)  # doesn't work
## 	EXECUTE_PROCESS(COMMAND find /usr/lib/ -name "omp.h"
## 	                COMMAND tail -n 1
## 	                COMMAND xargs dirname
## 	                OUTPUT_VARIABLE LATEST_OMP_H_DIR)
## 	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I${LATEST_OMP_H_DIR}")
## ENDIF()
## That's not working!
## A special version of Clang with OpenMP activated seems to be required.
## We just pasted gcc's "omp.h" into ext/openmp/include

# Turn some warnings off even in debug mode
IF(NOT RELEASE)
	# From Z3 library "check_context()" and others:
	# "unused parameter 'a'"...
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-parameter")
ENDIF()
IF(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
	# From MuParser-2.2.5 library -- http://muparser.beltoforion.de/:
	# "anonymous types declared in an anonymous union"
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-nested-anon-types")
	# From flex-2.5 generated file "lex.yy.cc":
	# 'register' storage class specifier is deprecated
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated-register")
ENDIF()
# From MuParser-2.2.5 library -- http://muparser.beltoforion.de/:
# "auto_ptr is deprecated" (TODO: update library ourselves?)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated-declarations")



###  Dynamic libraries specification  #########################################

# Allow project-local library paths
SET(CMAKE_LIBRARY_PATH ${PROJECT_SOURCE_DIR}/lib ${CMAKE_LIBRARY_PATH})

# Microsoft Research's SMT solver
FIND_LIBRARY(Z3 NAMES z3)

# Flex lexer (prefer static version)
FIND_LIBRARY(FLEX NAMES libfl.a fl)
IF(NOT FLEX)
	FIND_LIBRARY(FLEX NAMES fl)
	IF(NOT FLEX)
		SET(FLEX "")
	ENDIF()
ENDIF()

# Include all dynamic libraries to link against in a single variable
LIST(APPEND DYNAMIC_LIBRARIES ${FLEX} ${Z3})



###  Linker flags  ############################################################

# GNU pthreads
SET(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} -pthread" )

# OpenMP
SET(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} -fopenmp" )



###  Build executable  ########################################################

# FLEX lexer building
SET(LEXER_L  "${SOURCES_DIR}/Lexer.l")
SET(LEXER_CC "${PROJECT_BINARY_DIR}/lex.yy.cc")
ADD_CUSTOM_COMMAND(
	OUTPUT ${LEXER_CC}
	COMMAND flex++ ${LEXER_L}
	COMMAND sed -i "/#define yyFlexLexer yyFlexLexer/d" ${LEXER_CC}
	DEPENDS ${LEXER_L}
)

ADD_EXECUTABLE(fig ${ALL_SOURCES} ${LEXER_CC})



###  Link dynamic libraries  ##################################################

TARGET_LINK_LIBRARIES(fig ${DYNAMIC_LIBRARIES})
#TARGET_LINK_LIBRARIES(fig -static-libgcc -static-libstdc++ ${DYNAMIC_LIBRARIES})

