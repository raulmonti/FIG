//==============================================================================
//
//  ImportanceFunctionConcrete.cpp
//
//  Copyleft 2016-
//  Authors:
//  - Carlos E. Budde <cbudde@famaf.unc.edu.ar> (Universidad Nacional de Córdoba)
//
//------------------------------------------------------------------------------
//
//  This file is part of FIG.
//
//  The Finite Improbability Generator (FIG) project is free software;
//  you can redistribute it and/or modify it under the terms of the GNU
//  General Public License as published by the Free Software Foundation;
//  either version 3 of the License, or (at your option) any later version.
//
//  FIG is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//	GNU General Public License for more details.
//
//	You should have received a copy of the GNU General Public License
//	along with FIG; if not, write to the Free Software Foundation,
//	Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//==============================================================================


#include <ImportanceFunctionConcrete.h>
#include <Property.h>
#include <FigException.h>


namespace fig
{

ImportanceFunctionConcrete::ImportanceFunctionConcrete(const std::string& name) :
	ImportanceFunction(name),
	modulesConcreteImportance(1u)
{ /* Not much to do around here */ }


ImportanceFunctionConcrete::~ImportanceFunctionConcrete()
{
	clear();
}


void
ImportanceFunctionConcrete::assess_importance(const State<STATE_INTERNAL_TYPE>& s,
											  const Property& property,
											  const std::string& strategy,
											  const unsigned& index)
{
	if (modulesConcreteImportance.size() <= index)
		modulesConcreteImportance.resize(index);
	else if (modulesConcreteImportance[index].size() > 0u)
		throw_FigException(std::string("importance info already exists at ")
						   .append(" position").append(std::to_string(index)));

	// Compute importance following strategy
	if (  ""   == strategy ||
		"flat" == strategy) {
		ImportanceVec(s.concrete_size(), static_cast<ImportanceValue>(0)).swap(
				modulesConcreteImportance[index]);

	} else if ("auto" == strategy) {
		/// @todo TODO implement algorithm from sheet,
		///       the one with 'state[]' and 'redges[]' arrays.
		///       "modulesConcreteImportance[index]" would be the 'state[]'

	} else if ("adhoc" == strategy) {
		throw_FigException(std::string("importance strategy \"").append(strategy)
						   .append("\" isn't supported yet"));
	} else
		throw_FigException(std::string("unrecognized importance strategy \"")
						   .append(strategy).append("\""));
}


void
ImportanceFunctionConcrete::clear() noexcept
{
	for (unsigned i = 0u ; i < modulesConcreteImportance.size() ; i++)
		this->clear(i);
	std::vector<ImportanceVec>().swap(modulesConcreteImportance);
}


void
ImportanceFunctionConcrete::clear(const unsigned& index) noexcept
{
	if (modulesConcreteImportance.size() > index)
		ImportanceVec().swap(modulesConcreteImportance[index]);
		// Clear vector and ensure reallocation to 0 capacity
		// http://www.cplusplus.com/reference/vector/vector/clear/
}

} // namespace fig
