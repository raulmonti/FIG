/**
 * Instantiation of all basic TADs for debugging purposes.
 * This file is part of the FIG project.
 */

// C++
#include <set>
#include <list>
#include <tuple>
#include <vector>
#include <string>
#include <utility>
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
#include <State.h>
#include <MathExpression.h>
#include <Precondition.h>

// ADL
using std::to_string;
using std::make_tuple;
using std::get;


// Tests forward declarations
static void test_clock();
static void test_variable_interval();
static void test_variable_set();
static void test_state();
static void test_math_expression();
static void test_precondition();


int main()
{
	test_clock();
	test_variable_interval();
	test_variable_set();
	test_state();
	test_math_expression();
	test_precondition();

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
	TestException(const std::string& msg) : msg_(msg) {}
	TestException(std::string&& msg) : msg_(std::move(msg)) {}
};


static void // ////////////////////////////////////////////////////////////////
//
test_clock()
{
	fig::DistributionParameters params = {{2.0, 5.0}};
	fig::Clock c("uniformAB", params);
	assert(0.0 != c.sample() || 0.0 != c());  // exercise object
	try {
		fig::Clock c("unexistent_distribution", params);  // should throw
		throw TestException(to_string(__LINE__).append(": previous statement "
													   "should have thrown"));
	} catch (std::out_of_range) { /* this was expected */ }
}


static void // ////////////////////////////////////////////////////////////////
//
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
	v1 = v1.max() + 1; // should NOT throw, leaves v1 in invalid state
	v1 = v1.max();     // return v1 to valid state to avoid asserts
	try {
		v1.assign(v1.max() + 1); // should throw
		throw TestException(to_string(__LINE__).append(": previous statement "
													   "should have thrown"));
	} catch (fig::FigException) { /* this was expected */ }
	fig::VariableInterval<VI_TYPE> v2;  // fresh variable
	try {
		v2 = v1.min();  // should throw
		throw TestException(to_string(__LINE__).append(": previous statement "
													   "should have thrown"));
	} catch (fig::FigException) { /* this was expected */ }
	v2 = v1;
	try {
		v2 = v1; // should throw
		throw TestException(to_string(__LINE__).append(": previous statement "
													   "should have thrown"));
	} catch (fig::FigException) { /* this was expected */ }
	assert(v2 == v1);
	auto v3(v2);
	assert(v3 == v1);
	auto v4(fig::VariableInterval<VI_TYPE>(vname.append("different"), v1.min(), v1.max()));
	assert(v4 != v1);
}


static void // ////////////////////////////////////////////////////////////////
//
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
	w1 = w1.max() + 1; // should NOT throw, leaves w1 in invalid state
	w1 = w1.max();     // return w1 to valid state to avoid asserts
	try {
		w1.assign(w1.max() + 1); // should throw
		throw TestException(to_string(__LINE__).append(": previous statement "
													   "should have thrown"));
	} catch (fig::FigException) { /* this was expected */ }
	fig::VariableSet<VS_TYPE> w2(wname, wcontent.begin(), wcontent.end());
	assert(w2 != w1);  // current value of w1 is not the initial in wcontent
	fig::VariableSet<VS_TYPE> w3(w1);
	assert(w3 == w1);
	fig::VariableSet<VS_TYPE> w4;  // fresh variable
	try {
		w4 = w1.min();  // should throw
		throw TestException(to_string(__LINE__).append(": previous statement "
													   "should have thrown"));
	} catch (fig::FigException) { /* this was expected */ }
	w4 = w1;
	try {
		w4 = w1; // should throw
		throw TestException(to_string(__LINE__).append(": previous statement "
													   "should have thrown"));
	} catch (fig::FigException) { /* this was expected */ }
	std::list<VS_TYPE> w2content(wcontent.begin(), wcontent.end());
	fig::VariableSet<VS_TYPE> w5(wname, w2content);
	assert(w5 == w2);  // notice w2 was built from a set, and w5 from a list
}


static void // ////////////////////////////////////////////////////////////////
//
test_state()
{
	/// TODO
 ///
 ///  Complete this test
 ///
	typedef long TYPE;
	std::list<fig::VariableDeclaration<TYPE>> vars({
		make_tuple("x", 0, 10),
		make_tuple("y", -20, -19),
		make_tuple("ay_mama", 200, 4000001)
	});
	fig::GlobalState<TYPE> gState(vars);
	assert(gState.size() == vars.size());
	gState.print_out(std::cout, true);
	size_t i(0u);
	for (const auto& e: vars) {
		assert(get<0>(e) == gState[i]->name());
		assert(get<1>(e) == gState[i]->min());
		assert(get<2>(e) == gState[i]->max());
		++i;
	}
	auto s = gState.to_state_instance();
	assert(gState.is_valid_state_instance(*s));
	fig::GlobalState<TYPE> gState2(vars);
	fig::GlobalState<TYPE> gState3(std::move(gState));
	assert(gState  != gState3);
	assert(gState2 == gState3);
}


// Global variables needed/handy for expressions tests  ///////////////////////
//

const std::string expressionString1("x^y < x+y-1");
const std::set<std::string> varnames1({"x","y"});

std::set< fig::VariableDeclaration< fig::STATE_INTERNAL_TYPE > > vars({
	make_tuple("x", -1912, -1000),
	make_tuple("y", 0, 12),
	make_tuple("otra", 13, 14)
});
// MathExpression requires an externally defined GlobalState "fig::gState"
namespace fig
{
	GlobalState< fig::STATE_INTERNAL_TYPE > gState(vars);
}

static void // ////////////////////////////////////////////////////////////////
//
test_math_expression()
{
	/// TODO
 ///
 ///  Complete this test
 ///
	fig::MathExpression expr1(expressionString1, varnames1);
	assert(expressionString1 == expr1.expression());
}


static void // ////////////////////////////////////////////////////////////////
//
test_precondition()
{
	/// TODO
 ///
 ///  Complete this test
 ///
	fig::Precondition pre1(expressionString1, varnames1);
	assert(expressionString1 == pre1.expression());
}
