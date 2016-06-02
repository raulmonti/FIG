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
using std::begin;
using std::end;


/**
 * Functions offered to the end user for mathematical expressions
 */
namespace   // // // // // // // // // // // // // // // // // // // // // //
{

/// @todo: TODO extend MuParser library to accept functions with either
///        initializer lists as min<> and max<> below (try that first),
///        or std::vector<>, or else with variadic arguments.

/// Minimum between N values
template< typename T_ >
inline T_ min(std::initializer_list<T_> parameters)
{
	return std::min<T_>(parameters);
}

/// Maximum between N values
template< typename T_ >
inline T_ max(std::initializer_list<T_> parameters)
{
	return std::max<T_>(parameters);
}

/// Minimum between 2 values
template< typename T_ >
inline T_ min2(T_ a, T_ b)
{
	return std::min<T_>(a, b);
}

/// Maximum between 2 values
template< typename T_ >
inline T_ max2(T_ a, T_ b)
{
	return std::max<T_>(a, b);
}

} // namespace  // // // // // // // // // // // // // // // // // // // // // //



namespace fig  // // // // // // // // // // // // // // // // // // // // // //
{

template< template< typename, typename... > class Container,
          typename ValueType,
          typename... OtherContainerArgs >
MathExpression::MathExpression(
    const std::string& exprStr,
    const Container<ValueType, OtherContainerArgs...>& varnames) :
		empty_(trim(exprStr).empty()),
		exprStr_(muparser_format(exprStr)),
		pinned_(false)
{
    static_assert(std::is_constructible< std::string, ValueType >::value,
                  "ERROR: type mismatch. MathExpression needs a container "
                  "with variable names");
    // Setup MuParser expression
    parse_our_expression();
	// Register our variables names
	varsNames_.reserve(std::distance(begin(varnames), end(varnames)));
	for (const auto& name: varnames)
		if (exprStr_.find(name) != std::string::npos)
			varsNames_.emplace_back(name);  // copy elision
	varsNames_.shrink_to_fit();
	NVARS_ = varsNames_.size();
	// Positions mapping is done later in pin_up_vars()
	varsPos_.resize(NVARS_);
	varsValues_.resize(NVARS_);
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
        exprStr_(muparser_format(exprStr)),
		pinned_(false)
{
    static_assert(std::is_constructible< std::string, ValueType >::value,
                  "ERROR: type mismatch. MathExpression needs a container "
                  "with variable names");
    // Setup MuParser expression
    parse_our_expression();
	// Register our variables names
	varsNames_.reserve(NVARS_);
	for (const auto& name: varnames)
		if (exprStr_.find(name) != std::string::npos)
			varsNames_.emplace_back(name);
	varnames.clear();
	varsNames_.shrink_to_fit();
	NVARS_ = varsNames_.size();
	// Positions mapping is done later in pin_up_vars()
	varsPos_.resize(NVARS_);
	varsValues_.resize(NVARS_);
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

/// @todo TODO erase debug code
// MathExpression::MathExpression(const MathExpression& that) :
// 	empty_(that.empty_),
// 	exprStr_(that.exprStr_),
// 	expr_(that.expr_),
// 	varsMap_(that.varsMap_),
// 	pinned_(that.pinned_)
// { std::cerr << "ME: copy ctor\n"; }
// MathExpression::MathExpression(MathExpression&& that) :
// 	empty_(std::move(that.empty_)),
// 	exprStr_(std::move(that.exprStr_)),
// 	expr_(std::move(that.expr_)),
// 	varsMap_(std::move(that.varsMap_)),
// 	pinned_(std::move(that.pinned_))
// { std::cerr << "ME: move ctor\n"; }
////////////////////////////////


MathExpression& MathExpression::operator=(MathExpression that)
{
	std::swap(empty_, that.empty_);
	std::swap(exprStr_, that.exprStr_);
	std::swap(expr_, that.expr_);
	std::swap(varsNames_, that.varsNames_);
	std::swap(varsPos_, that.varsPos_);
//	std::swap(varsValues_, that.varsValues_);  // unnecessary
	std::swap(pinned_, that.pinned_);
	return *this;
}


void
MathExpression::pin_up_vars(const fig::State<STATE_INTERNAL_TYPE>& globalState)
{
	for (size_t i = 0ul ; i < NVARS_ ; i++) {
		varsPos_[i] = globalState.position_of_var(varsNames_[i]);
		expr_.DefineVar(varsNames_[i], &varsValues_[i]);  // could be in ctor
	}
	pinned_ = true;
/// @todo TODO erase old code
//	for (const auto& pair: varsMap_)
//		expr_.DefineVar(pair.first, &state_[globalState.position_of_var(pair.first)]);
//	for(auto& pair: varsMap_) {
//		pair.second = globalState.position_of_var(pair.first);
//////////////////////////////
}


#ifndef NRANGECHK
void
MathExpression::pin_up_vars(const PositionsMap& globalVars)
{
	for (size_t i = 0ul ; i < NVARS_ ; i++) {
		varsPos_[i] = globalVars.at(varsNames_[i]);
		expr_.DefineVar(varsNames_[i], &varsValues_[i]);  // could be in ctor
	}
	pinned_ = true;
}
#else
void
MathExpression::pin_up_vars(PositionsMap& globalVars)
{
	for (size_t i = 0ul ; i < NVARS_ ; i++) {
		varsPos_[i] = globalVars[varsNames_[i]];
		expr_.DefineVar(varsNames_[i], &varsValues_[i]);  // could be in ctor
	}
	pinned_ = true;
}
#endif


std::string
MathExpression::muparser_format(const std::string& expr) const
{
	if (trim(expr).empty())
		return "true";
    std::string muParserExpr(expr);
	delete_substring(muParserExpr, "\'");
	delete_substring(muParserExpr, "\"");
	// It's easier to do this syntactic change than to define the operators
	replace_substring(muParserExpr, "&", "&&");  // '&' should always appear single
	replace_substring(muParserExpr, "|", "||");  // '|' should always appear single
	return muParserExpr;
}


void
MathExpression::parse_our_expression()
{
	assert(!exprStr_.empty());
	try {
		expr_.SetExpr(exprStr_);
		// Define constants
		expr_.DefineConst("true",  1);
		expr_.DefineConst("false", 0);
		// Define operators
		expr_.DefineInfixOprt("!", [](mu::value_type v)->mu::value_type{return !v;});
		// Define functions offered for variables
		expr_.DefineFun("min", min2<STATE_INTERNAL_TYPE>);
		expr_.DefineFun("max", max2<STATE_INTERNAL_TYPE>);
		/*
		 *  TODO: bind all offered functions over variables
		 *        Notice MuParser already has a few: http://muparser.beltoforion.de/
		 *        But some are only available for floating point internal types
		expr.DefineFun("MySqr", MySqr);
		expr.DefineFun("Uni01", Uni01);
		...
		*/
	} catch (mu::Parser::exception_type &e) {
		std::cerr << "Failed parsing expression" << std::endl;
		std::cerr << "    message:  " << e.GetMsg()   << std::endl;
		std::cerr << "    formula:  " << e.GetExpr()  << std::endl;
		std::cerr << "    token:    " << e.GetToken() << std::endl;
		std::cerr << "    position: " << e.GetPos()   << std::endl;
		std::cerr << "    errc:     " << e.GetCode()  << std::endl;
		throw_FigException("bad mathematical expression");
	}
}

} // namespace fig  // // // // // // // // // // // // // // // // // // // //
