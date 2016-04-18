
/****************************************************************************** 
 * 
 *  file:  XorHandler.h
 * 
 *  Copyright (c) 2003, Michael E. Smoot .
 *  Copyright (c) 2004, Michael E. Smoot, Daniel Aarno.
 *  All rights reverved.
 * 
 *  See the file COPYING in the top directory of this distribution for
 *  more information.
 *  
 *  THE SOFTWARE IS PROVIDED _AS IS_, WITHOUT WARRANTY OF ANY KIND, EXPRESS 
 *  OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
 *  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 *  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
 *  DEALINGS IN THE SOFTWARE.  
 *
 *****************************************************************************
 *
 *  Extended by Carlos E. Budde on April 2016 <cbudde@famaf.unc.edu.ar>,
 *  FaMAF, Universidad Nacional de Córdoba, Argentina.
 *  All rights go to Michael E. Smoot.
 *
 *****************************************************************************/

#ifndef TCLAP_XORHANDLER_H
#define TCLAP_XORHANDLER_H

#include "Arg.h"
#include <string>
#include <vector>
#include <algorithm>
#include <iostream>

namespace TCLAP {

/**
 * This class handles lists of Arg's that are to be XOR'd or OR'd
 * on the command line.
 * @warning This is used by CmdLine and you shouldn't ever use it.
 */
class XorHandler
{
	protected:

		/**
		 * Vector with the lists of Arg's to be XOR'd/OR'd together.
		 */
		std::vector< std::pair< std::vector<Arg*>, bool> > _orList;

	public:

		/**
		 * Constructor.  Does nothing.
		 */
		XorHandler( ) : _orList() {}

		/**
		 * Add a list of Arg*'s that will be XOR'd/OR'd together.
		 * \param ors - list of Arg* that will be XOR'd/OR'd.
		 * \param allowMultiple - Result when more than one argument is given.
		 *        Defaults to 'false' thus behaving like the "XOR" this
		 *        class pledges. If 'true' then it'll behave like an "OR"
		 *        for the given 'ors' vector.
		 */
		void add(std::vector<Arg*>& ors , bool allowMultiple = false);
			
		/**
		 * Checks whether the specified Arg is in one of the xor lists and
		 * if it does match one, returns the size of the xor list that the
		 * Arg matched.  If the Arg matches, then it also sets the rest of
		 * the Arg's in the list. You shouldn't use this.  
		 * \param a - The Arg to be checked.
		 */
		int check( const Arg* a );

		/**
		 * Returns the XOR specific short usage.
		 */
		std::string shortUsage();

		/**
		 * Prints the XOR specific long usage.
		 * \param os - Stream to print to.
		 */
		void printLongUsage(std::ostream& os);

		/**
		 * Simply checks whether the Arg is contained in one of the arg
		 * lists.
		 * \param a - The Arg to be checked.
		 */
		bool contains( const Arg* a );

		const std::vector<std::pair<std::vector<Arg *>, bool> >&
		getXorList() const;

};


//////////////////////////////////////////////////////////////////////
//BEGIN XOR.cpp
//////////////////////////////////////////////////////////////////////
inline void XorHandler::add(std::vector<Arg*>& ors, bool allowMultiple )
{ 
	_orList.push_back( std::make_pair(ors, allowMultiple) );
}

inline int XorHandler::check( const Arg* a ) 
{
	// iterate over each XOR list
	for ( int i = 0; static_cast<unsigned int>(i) < _orList.size(); i++ )
	{
		// if the XOR list contains the arg..
		ArgVectorIterator ait = std::find( _orList[i].first.begin(),
										   _orList[i].first.end(), a );
		if ( ait != _orList[i].first.end() )
		{
			// for XOR, first check to see if a mutually exclusive switch
			// has already been set
			for ( ArgVectorIterator it = _orList[i].first.begin();
				  !_orList[i].second && it != _orList[i].first.end();
				  it++ )
				if ( a != (*it) && (*it)->isSet())
					throw(CmdLineParseException(
					      "Mutually exclusive argument already set!",
					      (*it)->toString()));

			// go through and set each arg that is not 'a':
			//   ·  OR => no more values will be required for this list
			//   · XOR => no more values will be allowed  for this list
			for ( ArgVectorIterator it = _orList[i].first.begin();
				  it != _orList[i].first.end();
				  it++ )
			{
				if ( a != (*it) ) {
					if ( _orList[i].second )
						(*it)->allowMore();
					else
						(*it)->xorSet();
				}
			}

			// return the number of required command line arguments
			// that have now been set
			if ( (*ait)->allowMore() )
				return 0;
			else
				return static_cast<int>(_orList[i].first.size());
		}
	}

	if ( a->isRequired() )
		return 1;
	else
		return 0;
}

inline bool XorHandler::contains( const Arg* a )
{
	for ( int i = 0; static_cast<unsigned int>(i) < _orList.size(); i++ )
		for ( ArgVectorIterator it = _orList[i].first.begin();
			  it != _orList[i].first.end();
			  it++ )	
			if ( a == (*it) )
				return true;

	return false;
}

const std::vector< std::pair< std::vector<Arg*>, bool> >&
XorHandler::getXorList() const
{
	return _orList;
}



//////////////////////////////////////////////////////////////////////
//END XOR.cpp
//////////////////////////////////////////////////////////////////////

} //namespace TCLAP

#endif 
