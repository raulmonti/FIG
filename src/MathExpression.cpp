//==============================================================================
//
//  MathExpression.cpp
//
//  Copyleft 2015-
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


// C++
#include <set>
#include <list>
#include <deque>
#include <vector>
#include <forward_list>
#include <unordered_set>
#include <iostream>
#include <algorithm>  // std::min<>(), std::max<>()
#include <initializer_list>
// C
#include <cstdarg>  // va_start, va_arg
// FIG
#include <MathExpression.h>
#include <string_utils.h>

// ADL
using std::swap;


namespace fig  // // // // // // // // // // // // // // // // // // // // // //
{

template< template< typename, typename... > class Container,
          typename ValueType,
          typename... OtherContainerArgs >
MathExpression::MathExpression(
    const std::string& exprStr,
    const Container<ValueType, OtherContainerArgs...>& varnames) :
		empty_(trim(exprStr).empty()),
        exprStr_(exprtk_format(exprStr)),
		pinned_(false)
{
    static_assert(std::is_constructible< std::string, ValueType >::value,
                  "ERROR: type mismatch. MathExpression needs a container "
                  "with variable names");
	// Register our variables names
	varsNames_.reserve(std::distance(begin(varnames), end(varnames)));
	for (const auto& name: varnames)
		if (std::find(begin(varsNames_),end(varsNames_),name) == end(varsNames_)
				&& exprStr_.find(name) != std::string::npos)
			varsNames_.emplace_back(name);  // copy elision
	varsNames_.shrink_to_fit();
	NVARS_ = varsNames_.size();
	// Positions mapping is done later in pin_up_vars()
	varsPos_.resize(NVARS_);
	varsValues_.resize(NVARS_);
    compile_expression();
}

// MathExpression can only be constructed with the following lvalue containers
template MathExpression::MathExpression(const std::string&,
                                        const std::set<std::string>&);
template MathExpression::MathExpression(const std::string&,
                                        const std::list<std::string>&);
template MathExpression::MathExpression(const std::string&,
                                        const std::deque<std::string>&);
template MathExpression::MathExpression(const std::string&,
                                        const std::vector<std::string>&);
template MathExpression::MathExpression(const std::string&,
                                        const std::forward_list<std::string>&);
template MathExpression::MathExpression(const std::string&,
                                        const std::unordered_set<std::string>&);


template< template< typename, typename... > class Container,
          typename ValueType,
          typename... OtherContainerArgs >
MathExpression::MathExpression(
    const std::string& exprStr,
    Container<ValueType, OtherContainerArgs...>&& varnames) :
		empty_(trim(exprStr).empty()),
        exprStr_(exprtk_format(exprStr)),
		pinned_(false)
{
    static_assert(std::is_constructible< std::string, ValueType >::value,
                  "ERROR: type mismatch. MathExpression needs a container "
                  "with variable names");
	// Register our variables names
	varsNames_.reserve(std::distance(begin(varnames), end(varnames)));
	for (const auto& name: varnames)
		if (std::find(begin(varsNames_),end(varsNames_),name) == end(varsNames_)
				&& exprStr_.find(name) != std::string::npos)
			varsNames_.emplace_back(name);
	varnames.clear();
	varsNames_.shrink_to_fit();
	NVARS_ = varsNames_.size();
	// Positions mapping is done later in pin_up_vars()
	varsPos_.resize(NVARS_);
	varsValues_.resize(NVARS_);
    compile_expression();
}

// MathExpression can only be constructed with the following rvalue containers
template MathExpression::MathExpression(const std::string&,
                                        std::set<std::string>&&);
template MathExpression::MathExpression(const std::string&,
                                        std::list<std::string>&&);
template MathExpression::MathExpression(const std::string&,
                                        std::deque<std::string>&&);
template MathExpression::MathExpression(const std::string&,
                                        std::vector<std::string>&&);
template MathExpression::MathExpression(const std::string&,
                                        std::forward_list<std::string>&&);
template MathExpression::MathExpression(const std::string&,
                                        std::unordered_set<std::string>&&);

void MathExpression::compile_expression() {
    for (size_t i = 0ul; i < NVARS_; i++) {
        table_.add_variable(varsNames_[i], varsValues_[i]);
    }
    expr_.register_symbol_table(table_);
    if (!parser.compile(exprStr_, expr_)) {
        throw_FigException("MathExpression: Couldn't parse expression");
    }
}

/// @deprecated
/// @see ExpState
void
MathExpression::pin_up_vars(const fig::State<STATE_INTERNAL_TYPE>& globalState)
{
	for (size_t i = 0ul ; i < NVARS_ ; i++) {
		varsPos_[i] = globalState.position_of_var(varsNames_[i]);
	}
	pinned_ = true;
}


#ifndef NRANGECHK
void
MathExpression::pin_up_vars(const PositionsMap& globalVars)
{
	for (size_t i = 0ul ; i < NVARS_ ; i++) {
		varsPos_[i] = globalVars.at(varsNames_[i]);
	}
	pinned_ = true;
}
#else
void
MathExpression::pin_up_vars(PositionsMap& globalVars)
{
	for (size_t i = 0ul ; i < NVARS_ ; i++) {
		varsPos_[i] = globalVars[varsNames_[i]];
	}
	pinned_ = true;
}
#endif


std::string
MathExpression::exprtk_format(const std::string& expr) const
{
	if (trim(expr).empty())
		return "true";
    std::string result(expr);
    delete_substring(result, "\'");
    delete_substring(result, "\"");
	// It's easier to do this syntactic change than to define the operators
    //replace_substring(muParserExpr, "&", "&&");  // '&' should always appear single
    //replace_substring(muParserExpr, "|", "||");  // '|' should always appear single
    return result;
}

} // namespace fig  // // // // // // // // // // // // // // // // // // // //
