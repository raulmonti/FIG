//==============================================================================
//
//  MultiDoubleArg.h
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
//------------------------------------------------------------------------------
//
//  This file is an extension to the Templatized C++ Command Line Parser
//  by Michael E. Smoot (TCLAP library, Copyright (c) 2003-2011)
//  All credit regarding this single file should go to him.
//
//==============================================================================

#ifndef MULTIDOUBLEARG_H
#define MULTIDOUBLEARG_H

// C++
#include <string>
#include <vector>
// External code
#include <Arg.h>
#include <Constraint.h>


namespace TCLAP
{

/**
 * A labelled double-argument that can be specified multiple times.
 * Each time the flag/name is matched on the command line, a parsing of
 * <b>two</b> values (of type "T1_" and "T2_" respectively) will be attempted.
 * This is basically the same as MultiArg, but each occurrence of the flag
 * must be followed by two values, e.g. "--stop-time 3 h"
 */
template< typename T1_, typename T2_ >
class MultiDoubleArg : public Arg
{
public:

	typedef typename std::vector<std::pair<T1_,T2_>>::const_iterator const_iterator;

protected:

	/// Values parsed from the CmdLine.
	std::vector< std::pair< T1_, T2_ > > _values;

	/// Description of the types T1_ and T2_ to be displayed in the usage.
	std::string _typeDesc;

	/// Constraints on the values
	Constraint<T1_>* _constraint1;
	Constraint<T2_>* _constraint2;

	/// Extract the values from the strings attempting to parse them
	/// as type T1_ and type T2_ respectively
	/// @param val1 - String to be read and parsed as type T1_
	/// @param val2 - String to be read and parsed as type T2_
	/// @throw ArgParseException if the types parsing fails
	void _extractValues(const std::string& val1,
						const std::string& val2);

	/// Used by XorHandler to decide whether to keep parsing for this arg.
	bool _allowMore;

public:  // Ctors/Dtor

	/**
	 * @brief Ctor with type descripion but no real constraints
	 * @param flag - The one character flag that identifies this
	 *               argument on the command line.
	 * @param name - A one word name for the argument.  Can be
	 *               used as a long flag on the command line.
	 * @param desc - A description of what the two arguments are for or do
	 * @param req  - Whether the argument is required on the command line
	 * @param typeDesc - A short, human readable description of the two types
	 *                   this object expects.  This is used in the generation
	 *                   of the USAGE statement.  The goal is to be helpful
	 *                   to the end user of the program.
	 */
	MultiDoubleArg(const std::string& flag,
				   const std::string& name,
				   const std::string& desc,
				   bool req,
				   const std::string& typeDesc);

	/**
	 * @brief Ctor with constraints on the two expected arguments
	 * @param flag - The one character flag that identifies this
	 *               argument on the command line.
	 * @param name - A one word name for the argument.  Can be
	 *               used as a long flag on the command line.
	 * @param desc - A description of what the two arguments are for or do
	 * @param req  - Whether the argument is required on the command line
	 * @param constraint1 - Pointer to a Constraint for the first  argument
	 * @param constraint2 - Pointer to a Constraint for the second argument
	 */
	MultiDoubleArg(const std::string& flag,
				   const std::string& name,
				   const std::string& desc,
				   bool req,
				   Constraint<T1_>* constraint1,
				   Constraint<T2_>* constraint2);

private:

	/// Prevent accidental copying
	MultiDoubleArg<T1_,T2_>(const MultiDoubleArg<T1_,T2_>& rhs);
	/// Prevent accidental copying
	MultiDoubleArg<T1_,T2_>& operator=(const MultiDoubleArg<T1_,T2_>& rhs);

public:

	/// Default dtor
	virtual ~MultiDoubleArg();

public:  // Utils

	/**
	 * Handles the processing of the two arguments.
	 * This re-implements the Arg version of this method to set the
	 * _value of the argument appropriately.  It knows the difference
	 * between labeled and unlabeled.
	 * \param i - Pointer the the current argument in the list.
	 * \param args - Mutable list of strings. Passed from main().
	 */
	virtual bool processArg(int* i, std::vector<std::string>& args);

	/// Vector with the value pairs parsed from the command line
	inline const std::vector< std::pair< T1_, T2_ > >& getValues()
		{ return _values; }

	/// Iterator pointing to the first value pair parsed from the command line
	inline const_iterator begin() const
		{ return _values.begin(); }

	/// Iterator past the last value pair parsed from the command line
	inline const_iterator end() const
		{ return _values.end(); }

	/// Short id string used in the USAGE printing
	/// \param val - value to be used.
	inline virtual std::string shortID(const std::string&) const
		{ return Arg::shortID(_typeDesc) + " ... "; }

	/// Long id string used in the USAGE printing
	/// \param val - value to be used.
	inline virtual std::string longID(const std::string&) const
		{ return Arg::shortID(_typeDesc) + " ... "; }

	/// @copydoc Arg::isRequired()
	/// @note Notice once the first value has been matched, new occurrences
	///       of this MultiDoubleArg are no longer required
	inline virtual bool isRequired() const
		{ return _required && _values.empty(); }

	virtual bool allowMore();

	virtual void reset();
};



// // // // // // // // // // // // // // // // // // // // // // // // // //
//
//  Implementation of the template class function members
//
// // // // // //
//
//  Code bloating should be insignificant since this file is only supposed
//  to be used by the FIG's CLI parser, i.e. it should only be included in
//  fig_cli.cpp
//


template<typename T1_, typename T2_>
MultiDoubleArg<T1_,T2_>::MultiDoubleArg(
	const std::string& flag,
	const std::string& name,
	const std::string& desc,
	bool req,
	const std::string& typeDesc) :
		Arg(flag, name, desc, req, true, nullptr),
		_values(),
		_typeDesc(typeDesc),
		_constraint1(nullptr),
		_constraint2(nullptr),
		_allowMore(false)
{
	_acceptsMultipleValues = true;
}


template<typename T1_, typename T2_>
MultiDoubleArg<T1_,T2_>::MultiDoubleArg(
	const std::string& flag,
	const std::string& name,
	const std::string& desc,
	bool req,
	Constraint<T1_>* constraint1,
	Constraint<T2_>* constraint2) :
		Arg(flag, name, desc, req, true, nullptr),
		_values(),
		_typeDesc(constraint1->shortID() + "> <" + constraint2->shortID()),
		_constraint1(constraint1),
		_constraint2(constraint2),
		_allowMore(false)
{
	_acceptsMultipleValues = true;
}


template< typename T1_, typename T2_ >
MultiDoubleArg<T1_,T2_>::~MultiDoubleArg()
{
	reset();
}


template< typename T1_, typename T2_ >
void MultiDoubleArg<T1_,T2_>::_extractValues(const std::string& val1,
											 const std::string& val2)
{
	std::pair<T1_,T2_> tmp;
	try {
		/// @todo FIXME check for troubles, maybe the first invocation swallows both values an fails
		ExtractValue(tmp.first,  val1, typename ArgTraits<T1_>::ValueCategory());
		ExtractValue(tmp.second, val2, typename ArgTraits<T2_>::ValueCategory());
	} catch (ArgParseException &e) {
		const std::string MARGIN("             ");
		throw ArgParseException(e.error() + ".\n"
								+ MARGIN + "Argument \"--" + _name + "\" takes "
								"two values; if you provided one (or none)\n"
								+ MARGIN + "then another argument's name or "
								"value could've been used.", toString());
	}
	if (_constraint1 != nullptr && !_constraint1->check(tmp.first))
		throw CmdLineParseException("Value '" + val1 + "' fails to meet a "
									"constraint: " + _constraint1->description(),
									toString());
	if (_constraint2 != nullptr && !_constraint2->check(tmp.second))
		throw CmdLineParseException("Value '" + val2 + "' fails to meet a "
									"constraint: " + _constraint2->description(),
									toString());
	_values.push_back(tmp);
}


template< typename T1_, typename T2_ >
bool
MultiDoubleArg<T1_,T2_>::processArg(int *i, std::vector<std::string>& args)
{
	if (_ignoreable && Arg::ignoreRest())
		return false;

	if (_hasBlanks(args[*i]))
		return false;

	std::string flag = args[*i];
	std::string value = "";
	trimFlag(flag, value);

	if (argMatches(flag)) {

        if (Arg::delimiter() != ' ' && value == "")
            throw ArgParseException("Couldn't find delimiter for this argument!",
                                    toString());
        if (value != "")
            throw ArgParseException("This argument takes two values but "
                                    "nothing could be parsed", toString());
        if ((*i)+1ul >= args.size())
            throw ArgParseException("This argument takes two values yet "
                                    "none was provided", toString());
        if ((*i)+2ul >= args.size())
            throw ArgParseException("This argument takes two values but "
                                    "only one was provided", toString());
        // take the first two subsequent strings, regardless of start string
        _extractValues(args[++(*i)], args[++(*i)]);

        _alreadySet = true;
        return true;
    }

    return false;
}


template< typename T1_, typename T2_ >
bool
MultiDoubleArg<T1_,T2_>::allowMore()
{
	bool am(_allowMore);
	_allowMore = true;
	return am;
}


template< typename T1_, typename T2_ >
void
MultiDoubleArg<T1_,T2_>::reset()
{
	Arg::reset();
	_values.clear();
}

} // namespace TCLAP

#endif // MULTIDOUBLEARG_H
