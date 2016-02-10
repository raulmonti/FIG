##==============================================================================
##
##  CMakeLists.txt
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

CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

PROJECT (fig_test_tads)

SET(fig_VERSION_MAYOR 0)
SET(fig_VERSION_MINOR 4)



###  Project directory structure  #############################################

# Default directories configuration
SET(SOURCES_DIR ${PROJECT_SOURCE_DIR}/../../src)
SET(HEADERS_DIR ${PROJECT_SOURCE_DIR}/../../include)
SET(LIBRARIES_DIR ${PROJECT_SOURCE_DIR}/../../lib)
SET(PROJECT_BINARY_DIR ${PROJECT_SOURCE_DIR}/../../bin)
SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/test)

# Header file with CMake settings for the source code
CONFIGURE_FILE (
	"${SOURCES_DIR}/FigConfig.h.in"
	"${HEADERS_DIR}/FigConfig.h"
)

# Allow subdirectories to append their files and dirs (https://goo.gl/xxIOAM)
SET(HEADERS_DIR ${HEADERS_DIR} CACHE INTERNAL "")
SET(ALL_SOURCES CACHE INTERNAL "")

# Visit necessary subdirectories
ADD_SUBDIRECTORY(${PROJECT_SOURCE_DIR}/../../ext/muparser2.2.5
                 ${PROJECT_BINARY_DIR}/muparser2.2.5)
ADD_SUBDIRECTORY(${PROJECT_SOURCE_DIR}/../../ext/openmp
                 ${PROJECT_BINARY_DIR}/openmp)
 
# Register all include paths
INCLUDE_DIRECTORIES(${HEADERS_DIR})

# Register the source files (only those required for this test)
SET(REQUIRED_SOURCES "${SOURCES_DIR}/Clock.cpp"
                     "${SOURCES_DIR}/Variable.cpp"
                     "${SOURCES_DIR}/VariableInterval.cpp"
                     "${SOURCES_DIR}/VariableSet.cpp"
                     "${SOURCES_DIR}/State.cpp"
                     "${SOURCES_DIR}/MathExpression.cpp"
                     "${SOURCES_DIR}/Precondition.cpp"
                     "${SOURCES_DIR}/Postcondition.cpp"
                     "${SOURCES_DIR}/Transition.cpp"
                     "${SOURCES_DIR}/ModuleInstance.cpp"
                     "${SOURCES_DIR}/ModuleNetwork.cpp"
                     "${SOURCES_DIR}/ModelSuite.cpp"
                     "${SOURCES_DIR}/Traial.cpp"
                     "${SOURCES_DIR}/TraialPool.cpp"
                     "${SOURCES_DIR}/PropertyTransient.cpp"
                     "${SOURCES_DIR}/SimulationEngine.cpp"
                     "${SOURCES_DIR}/SimulationEngineNosplit.cpp"
                     "${SOURCES_DIR}/ImportanceFunction.cpp"
                     "${SOURCES_DIR}/ImportanceFunctionConcrete.cpp"
                     "${SOURCES_DIR}/ImportanceFunctionConcreteCoupled.cpp"
                     "${SOURCES_DIR}/ConfidenceInterval.cpp"
                     "${SOURCES_DIR}/ConfidenceIntervalMean.cpp"
                     "${SOURCES_DIR}/ConfidenceIntervalWilson.cpp"
                     "${SOURCES_DIR}/ConfidenceIntervalProportion.cpp")
SET(ALL_SOURCES ${ALL_SOURCES} ${REQUIRED_SOURCES} CACHE INTERNAL "")

# Register also the source files from this test
AUX_SOURCE_DIRECTORY(${PROJECT_SOURCE_DIR} TEST_SOURCES)
SET(ALL_SOURCES ${ALL_SOURCES} ${TEST_SOURCES} CACHE INTERNAL "")

## # Libraries
## LINK_DIRECTORIES(${LIBRARIES_DIR})
##  Tip: use find_library() and find_package() instead


 
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

# Highly defensive compile flags
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic")

## # Debug
## SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")

## # No-Debug
## SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DNDEBUG")

# OpenMP
IF(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
##	FIND_PACKAGE(OpenMP REQUIRED)  # doesn't work
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fopenmp=libgomp")
ELSE()
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fopenmp")
ENDIF()
## # Clang needs to be told where to look for OpenMP headers
## IF(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
## 	EXECUTE_PROCESS(COMMAND find /usr/lib/ -name "omp.h"
## 	                COMMAND tail -n 1
## 	                COMMAND xargs dirname
## 	                OUTPUT_VARIABLE LATEST_OMP_H_DIR)
## 	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I${LATEST_OMP_H_DIR}")
## ENDIF()
## That's not working!
## A special version of Clang with OpenMP activated seems to be required.
## We just pasted gcc's "omp.h" into ext/openmp/include

# Turn some warnings off
IF(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
	# "anonymous types declared in an anonymous union"
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-nested-anon-types")
ENDIF()
# Avoid MuParser warnings due to deprecated auto_ptr (TODO: update library?)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated-declarations")



###  Link flags  ##############################################################

# GNU pthreads
SET(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} -pthread" )

# OpenMP
SET(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} -fopenmp" )



###  Build executable  ########################################################

ADD_EXECUTABLE(test_tads ${ALL_SOURCES})

