/**
 * Instantiation of all basic TADs for debugging purposes.
 * This file is part of the FIG project.
 */

// C++
#include <set>
#include <vector>
#include <string>
#include <iostream>
#include <exception>
// C
#include <cassert>
// Project code
#include <FigException.h>
#include <Clock.h>
#include <Variable.h>
#include <VariableSet.h>
#include <VariableInterval.h>

// Tests forward declarations
static void test_clock();
static void test_variable_interval();
static void test_variable_set();


int main()
{
	test_clock();
	test_variable_interval();
	test_variable_set();

	return 0;
}

// ////////////////////////////////////////////////////////////////////////////
class TestException : public std::exception
{
	std::string msg_;
	inline virtual const char* what() const throw()
	{
		return std::string("Exception in test: ").append(msg_).c_str();
	}
public:
	TestException(const char* msg) : msg_(msg) {}
};


static void // ////////////////////////////////////////////////////////////////

test_clock()
{
	fig::DistributionParameters params = {{2.0, 5.0}};
	fig::Clock c("uniformAB", params);
	assert(0.0 != c.sample() || 0.0 != c());  // exercise object
	try {
		fig::Clock c("unexistent_distribution", params);  // should throw
		throw TestException("previous definition should have thrown");
	}
	catch (std::out_of_range) {	/* this was expected */ }
}


static void // ////////////////////////////////////////////////////////////////

test_variable_interval()
{
	typedef unsigned long VI_TYPE;
	std::string vname("v");
	fig::VariableInterval<VI_TYPE> v1(vname, 0, 9);
	v1.assert_invariant();
	assert(v1 == v1);
	for (VI_TYPE l = v1.min() ; l <= v1.max() ; l++) {
		v1 = l;
		v1.assert_invariant();
	}
	try {
		v1 = v1.max() + 1; // should throw
		throw TestException("previous statement should have thrown");
	} catch (fig::FigException) { /* this was expected */ }
	fig::VariableInterval<VI_TYPE> v2;  // fresh variable
	try {
		v2 = v1.min();  // should throw
		throw TestException("previous statement should have thrown");
	} catch (fig::FigException) { /* this was expected */ }
	v2 = v1;
	try {
		v2 = v1; // should throw
		throw TestException("previous statement should have thrown");
	} catch (fig::FigException) { /* this was expected */ }
	assert(v2 == v1);
	auto v3(v2);
	assert(v3 == v1);
	auto v4(fig::VariableInterval<VI_TYPE>(vname.append("different"), v1.min(), v1.max()));
	assert(v4 != v1);
}


#include <list>
static void // ////////////////////////////////////////////////////////////////

test_variable_set()
{
	typedef short VS_TYPE;
	std::string wname("w");
//	short content[] = {0, 1, -1, 2, 0};
//	fig::VariableSet<VS_TYPE> w1(wname, content, 5);
	const std::set<VS_TYPE> wcontent({0,-12,-32767,32767,0});
	fig::VariableSet<VS_TYPE> w1(wname, wcontent);
	w1.assert_invariant();
	for (size_t i = 0 ; i < w1.range() ; i++) {
		w1 = w1.val(i);
		w1.assert_invariant();
	}
	try {
		w1 = w1.min() - 1; // should throw
		throw TestException("previous statement should have thrown");
	} catch (fig::FigException) { /* this was expected */ }
	fig::VariableSet<VS_TYPE> w2(wname, wcontent.begin(), wcontent.end());
	assert(w2 != w1);  // current value of w1 is not the initial in wcontent
	fig::VariableSet<VS_TYPE> w3(w1);
	assert(w3 == w1);
	fig::VariableSet<VS_TYPE> w4;  // fresh variable
	try {
		w4 = w1.min();  // should throw
		throw TestException("previous statement should have thrown");
	} catch (fig::FigException) { /* this was expected */ }
	w4 = w1;
	try {
		w4 = w1; // should throw
		throw TestException("previous statement should have thrown");
	} catch (fig::FigException) { /* this was expected */ }
	std::list<VS_TYPE> w2content(wcontent.begin(), wcontent.end());
	fig::VariableSet<VS_TYPE> w5(wname, w2content);
	assert(w5 == w2);  // notice w2 was built from a set, and w5 from a list
}

