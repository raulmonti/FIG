/**
 * Instantiation of all basic TADs for debugging purposes.
 * This file is part of the FIG project.
 */

// C++
#include <string>
#include <iostream>
#include <exception>
// C
#include <cassert>
// Project code
#include <FigException.h>
#include <Clock.h>
#include <Variable.h>
#include <VariableInterval.h>

// Tests forward declarations
static void test_clocks();
static void test_variables();


int main()
{
	test_clocks();
	test_variables();  // TODO extend for VariableSet

	return 0;
}


static void // ////////////////////////////////////////////////////////////////

test_clocks()
{
	fig::DistributionParameters params = {{2.0, 5.0}};
	fig::Clock c("uniformAB", params);
	assert(0.0 != c.sample() || 0.0 != c());  // exercise object
	try {
		fig::Clock c("unexistent_distribution", params);  // should throw
		throw fig::FigException("previous definition should have thrown");
	}
	catch (std::out_of_range) {	/* this was expected */ }
}


static void // ////////////////////////////////////////////////////////////////

test_variables()
{
	fig::VariableInterval<short> v("v", 0, 9);
	v.assert_invariant();
	assert(v == v);
	v = 1;
	try {
		v = 10; // should throw
		throw fig::FigException("previous definition should have thrown");
	}
	catch (fig::FigException) { /* this was expected */ }
	fig::VariableInterval<short> v2;
	v2 = v;
	try {
		v2 = v; // should throw
		throw fig::FigException("previous definition should have thrown");
	}
	catch (fig::FigException) { /* this was expected */ }
	assert(v2 == v);
	fig::VariableInterval<short> v3(v2);
	assert(v3 == v);
	std::string vname = v.name();
	auto v4(fig::VariableInterval<short> (vname.append("different"), v.min(), v.max()));
	assert(v4 != v);

	// TODO same checks for VariableSet
}

