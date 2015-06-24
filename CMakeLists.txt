##==============================================================================
##	
##	Copyleft 2015-
##	Authors:
##  * Carlos E. Budde <cbudde@famaf.unc.edu.ar> (Universidad Nacional de Córdoba)
##
##------------------------------------------------------------------------------
##
##  This file is part of fig.
##
##  The Finite Improbability Generator (fig) project is free software;
##  you can redistribute it and/or modify it under the terms of the GNU
##  General Public License as published by the Free Software Foundation;
##  either version 3 of the License, or (at your option) any later version.
##
##  fig is distributed in the hope that it will be useful,
##	but WITHOUT ANY WARRANTY; without even the implied warranty of
##	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
##	GNU General Public License for more details.
##	
##	You should have received a copy of the GNU General Public License
##	along with PRISM; if not, write to the Free Software Foundation,
##	Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
##
##==============================================================================

CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

PROJECT (fig)

SET(fig_VERSION_MAYOR 0)
SET(fig_VERSION_MINOR 1)
 
# Default directories configuration
SET(SOURCES_DIR ${PROJECT_SOURCE_DIR}/src)
SET(INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include)
SET(PROJECT_BINARY_DIR ${PROJECT_SOURCE_DIR}/bin)
SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/fig)
## MESSAGE("include dir: ${INCLUDE_DIR}")
## MESSAGE("sources dir: ${SOURCES_DIR}")
## MESSAGE("bin dir: ${PROJECT_BINARY_DIR}")
## MESSAGE("exe dir: ${EXECUTABLE_OUTPUT_PATH}")

# Header file with CMake settings for the source code
CONFIGURE_FILE (
	"${SOURCES_DIR}/FigConfig.h.in"
	"${INCLUDE_DIR}/FigConfig.h"
)
 
# Includes
INCLUDE_DIRECTORIES(${INCLUDE_DIR})
#INCLUDE_DIRECTORIES(${INCLUDE_DIR}/boost)

# Check for c++11 standard support
INCLUDE(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
IF(COMPILER_SUPPORTS_CXX11)
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
ELSE()
	MESSAGE(FATAL_ERROR "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support.")
ENDIF()


# Executable
ADD_EXECUTABLE(Tuto ${SOURCES_DIR}/bgl_tutorial.cpp)

