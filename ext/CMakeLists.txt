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

PROJECT (fig_ext)

# Include all direct subdirectories
ADD_SUBDIRECTORY(z3)
ADD_SUBDIRECTORY(openmp)
ADD_SUBDIRECTORY(gsl2.1)
ADD_SUBDIRECTORY(pcg0.98)
ADD_SUBDIRECTORY(uint128_t)
ADD_SUBDIRECTORY(tclap1.2.1)
ADD_SUBDIRECTORY(jsoncpp1.7.4)
#ADD_SUBDIRECTORY(muparser2.2.5)
ADD_SUBDIRECTORY(exprtk)

#ADD_SUBDIRECTORY(boost)

