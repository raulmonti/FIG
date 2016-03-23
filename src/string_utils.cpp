//==============================================================================
//
//  string_utils.cpp
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


// C
#include <cctype>  // std::isspace()
// C++
#include <algorithm>  // find_if_not()
// FIG
#include <string_utils.h>

#if __cplusplus < 201103L
#  error "C++11 standard required, please compile with -std=c++11\n"
#endif

using std::string;
using std::isspace;
using std::find_if_not;


string&
replace_substring(string& s, const string& from, const string& to)
{
	if (from.empty())
		return s;
	size_t start_pos(0ul);
	while ((start_pos = s.find(from, start_pos)) != string::npos) {
		s.replace(start_pos, from.length(), to);
		start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
	}
	return s;
}


std::string replace_substring(std::string &&s, const string& from, const string& to)
{
	string ss = replace_substring(s, from, to);
	return ss;
}


string&
delete_substring(string& s, const string& substr)
{
	return replace_substring(s, substr, "");
}


string
delete_substring(string&& s, const string& substr)
{
	string ss = delete_substring(s, substr);
	return s;
}


string
trim(const string &s)
{
	auto is_space = [](int c){ return isspace(c); };
	auto wsfront = find_if_not(s.begin(), s.end(), is_space);
	return string(wsfront, find_if_not(s.rbegin(),
									   string::const_reverse_iterator(wsfront),
									   is_space).base());
}


string
trim(string&& s)
{
	const string s2(s);
	return trim(s);
}
