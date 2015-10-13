#include <iostream>
#include <string>
#include <State.h>
#include <Ifun.h>

using std::cout;
using std::endl;
using std::string;


int main()
{
	// Data set for testing
	std::vector< VarDec<int> > vars;
	vars.reserve(4);
	vars.emplace_back("x", 0, 1);
	vars.emplace_back("y", 5, 10);
	vars.emplace_back("cnt", 7, 7);
	vars.emplace_back(vars[0]);
	for (unsigned i=0 ; i < vars.size() ; i++) {
		const auto& v = vars[i];
		cout << std::get<0>(v) << ": "
		     << std::get<1>(v) << " ∈ ["
		     << std::get<1>(v) << ","
		     << std::get<2>(v) << "]" << endl;
	}

	// Data ctor
	State<int> s0((State<int>(vars)));
	assert(s0.size() == vars.size());
	// Copy ctor
	State<int> s1(s0);
	assert(s0.compatible(s1));
	assert(s0 == s1);
	// Move ctor
	State<int> s2(std::move(s0));
	assert(0 == s0.size());
	assert(s1 == s2);
	assert(s0 != s1);
	// Copy assignment
	s0 = s2;
	assert(s0 == s2);
	assert(s0 == s1);
	// Move assignment
	s1 = std::move(s0);
	assert(0 == s0.size());
	assert(s0 != s1);
	assert(!s0.compatible(s1));
	assert(s1 == s2);
	// Value assignment
	s1[0] = s1[0].max();
	assert(s1.compatible(s2));
	assert(s1 != s2);
	// Find var by name
	auto v1 = s1["cnt"];
	assert(nullptr != v1);
	assert((*v1) == s1[2]);
	(*v1) = v1->max();
	assert((*v1) == s1[2]);
	v1 = s1["noexiste"];
	assert(nullptr == v1);
	
	return 0;
}
