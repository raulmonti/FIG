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
##	along with FIG; if not, write to the Free Software Foundation,
##	Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
##
##==============================================================================

PROJECT (fig_ext_muparser)
SET(CWD ${CMAKE_CURRENT_LIST_DIR})

# List include paths
SET(HEADERS_DIR
	${HEADERS_DIR}
	${CWD}/include
	CACHE INTERNAL ""  # append persistently for parent CMakeLists
)

# Register source files
AUX_SOURCE_DIRECTORY(${CWD}/src OUR_SOURCES)
SET(ALL_SOURCES ${ALL_SOURCES} ${OUR_SOURCES} CACHE INTERNAL "")

