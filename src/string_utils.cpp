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
#include <cstdio>  // std::getline()
#include <cassert>
// C++
#include <regex>
#include <sstream>
#include <algorithm>  // find_if_not(), search()
// FIG
#include <string_utils.h>

#if __cplusplus < 201103L
#  error "C++11 standard required, please compile with -std=c++11\n"
#endif

using std::string;
using std::isspace;
using std::find_if_not;
using namespace std::regex_constants;
const auto& NPOS(std::string::npos);


size_t
count(const string &s, const char &c)
{
	long cnt(-1l), pos(-1l);
	do {
		cnt++; pos++;
		pos = s.find(c, pos);
	} while (NPOS != static_cast<size_t>(pos));
	assert(cnt >= 0l);
	assert(cnt <= static_cast<long>(s.length()));
	return static_cast<size_t>(cnt);
}


bool is_substring(const string& str, const string& substr, const bool caseSensitive)
{
	return caseSensitive
	        ? str.find(substr) != NPOS
	        : str.end() != std::search(
	            str.begin(), str.end(), substr.begin(), substr.end(),
	            [](char c1, char c2) { return std::toupper(c1) == std::toupper(c2); } );
}


bool is_prefix(const string& str, const string& prefix, const bool caseSensitive)
{
	const std::regex re("^" + prefix + ".*$",
	                    caseSensitive ? ECMAScript : ECMAScript|icase);
	return std::regex_match(str, re);
}


bool is_suffix(const string& str, const string& suffix, const bool caseSensitive)
{
	const std::regex re("^.*" + suffix + "$",
	                    caseSensitive ? ECMAScript : ECMAScript|icase);
	return std::regex_match(str, re);
}


string&
replace_substring(string& s, const string& dis, const string& todis)
{
	if (dis.empty())
		return s;
	size_t start_pos(0ul);
	while ((start_pos = s.find(dis, start_pos)) != NPOS) {
		s.replace(start_pos, dis.length(), todis);
		start_pos += todis.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
	}
	return s;
}


std::string&
replace_substring(std::string &&s, const string& dis, const string& todis)
{
	return replace_substring(s, dis, todis);
}


string&
delete_substring(string& s, const string& substr)
{
	return replace_substring(s, substr, "");
}


std::string&
delete_substring(string&& s, const string& substr)
{
	return replace_substring(s, substr, "");
}


string
delete_substring(const std::string& s, const std::string& substr)
{
	string sCopy(s);
	return replace_substring(std::move(sCopy), substr, "");
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
trim(string& s)
{
	s = trim(const_cast<const string&>(s));
	return s;
}


string
trim(string&& s)
{
	const string s2(s);
	return trim(s2);
}


std::vector<std::string>
split(const std::string& s,
	  char delim,
	  bool includeEmptyMatches)
{
	std::vector<string> result;
	result.reserve( count(s,delim) + 1ul );
	std::stringstream ss(s);
	std::string item;
	while (std::getline(ss, item, delim))
		if (!item.empty() || includeEmptyMatches)
			result.push_back(item);
	return result;
}


bool
filename_has_extension(const std::string& filename,
					   const std::string& extension)
{
	assert(filename.length() > 0ul);
	assert(extension.length() > 0ul);
	const std::regex ext("^.*\\"
						+ (extension[0] == '.' ? string("") : string("."))
						+ extension + "$");
	return std::regex_match(filename, ext);
}


std::string
change_filename_extension(const std::string& filename,
						  const std::string& extension)
{
	assert(filename.length() > 0ul);
	assert(extension.length() > 0ul);
	string normalizedExt((extension[0] == '.' ? "" : ".") + extension);
	const auto extBegin = filename.find_last_of('.');
	if (NPOS == extBegin)
		return filename + normalizedExt;
	else
		return filename.substr(0, extBegin) + normalizedExt;
}

