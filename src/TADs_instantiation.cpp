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
// FIG
#include <FigException.h>
#include <Label.h>
#include <Clock.h>
#include <Variable.h>
#include <VariableSet.h>
#include <VariableInterval.h>
#include <State.h>
#include <MathExpression.h>
#include <Precondition.h>
#include <Postcondition.h>

using std::to_string;
using std::make_tuple;
using std::get;


// Tests forward declarations
static void test_label();
static void test_clock();
static void test_variable_interval();
static void test_variable_set();
static void test_state();
static void test_math_expression();
static void test_precondition();
static void test_postcondition();
static void test_transition();


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


int main()
{
	std::cout << "\nIgnore ALL following messages BUT the last line\n" << std::endl;

	try {
		test_label();
		test_clock();
		test_variable_interval();
		test_variable_set();
		test_state();
		test_math_expression();
		test_precondition();
		test_postcondition();
		test_transition();

	} catch (TestException& e) {
		std::cout << "Some test failed, details follow." << std::endl;
		throw e;

	} catch (fig::FigException& e) {
		std::cout << "Some test failed unexpectedly, details follow." << std::endl;
		throw e;
	}

	std::cout << "\nAll tests were successfull!\n" << std::endl;

	return 0;
}


static void // ////////////////////////////////////////////////////////////////
//
test_label()
{
	fig::Label tau;
	assert(tau.is_input());
	assert(tau.is_tau());

	fig::Label input("a", false);
	assert(!input.is_tau());
	assert(input.is_input());
	assert(tau != input);

	fig::Label output("a", true);
	assert(!output.is_tau());
	assert(!output.is_input());
	assert(tau != output);
	assert(input == output);
	assert(!output.same_as(input));
}


static void // ////////////////////////////////////////////////////////////////
//
test_clock()
{
	fig::DistributionParameters params = {{2.0, 5.0}};
    fig::Clock c("c", "uniformAB", params);
	assert(0.0 != c.sample() || 0.0 != c());  // exercise object
	try {
        fig::Clock c2("c2", "unexistent_distribution", params);  // should throw
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


// Global variables needed by some modules  ///////////////////////////////////
//

// MathExpression requires an externally defined GlobalState "fig::gState"
namespace fig
{
typedef fig::VariableInterval< fig::STATE_INTERNAL_TYPE > VarType;
GlobalState< STATE_INTERNAL_TYPE > gState(
	std::vector< VarType >{
		VarType("x", -1912, -1000),
		VarType("otra", 13, 14),
		VarType("y", 0, 12)
	});
}

// Transition requires an externally defined std::vector<Clock> "fig::gClocks"
namespace fig
{
std::vector<Clock> gClocks;
}


static void // ////////////////////////////////////////////////////////////////
//
test_math_expression()
{
	// Correct expressions
	const std::string str1("x^y > max(x,y)");
	const std::string str2("x=y, y=x^2");  // updates are accumulative: y = y^2
	const std::set<std::string> varnames({"x","y"});
	fig::MathExpression expr1(str1, varnames);
	assert(str1 == expr1.expression());
	fig::MathExpression expr2(str2, varnames);
	assert(str2 == expr2.expression());

	// Incorrect creation data
	const std::string str3("x-y-z < _pi^2");
	const std::set<std::string> varnames3({"x","y"});  // forgot "z"
	try {
		fig::MathExpression expr3(str3, varnames3);  // shouldn't throw anyway
		assert(str3 == expr3.expression());
	} catch (std::exception) {
		throw TestException(to_string(__LINE__).append(": previous statement "
													   "shouldn't have thrown!"));
	}
	const std::string str4("x+y == _pi-0");
	const std::set<std::string> varnames4({"x","y","z"});  // "z" doesn't exist
	try {
		fig::MathExpression expr4(str4, varnames4);
		throw TestException(to_string(__LINE__).append(": previous statement "
													   "should have thrown"));
	} catch (std::out_of_range) { /* this was expected */ }
}


static void // ////////////////////////////////////////////////////////////////
//
test_precondition()
{
	const std::string str1("x^y > max(x,y)");
	const std::set<std::string> varnames1({"x","y"});
	fig::Precondition pre1(str1, varnames1);
	assert(str1 == pre1.expression());

	// Positions of variables in State instances are determined by the
	// unique GlobalState 'gState' object
	fig::State s1 = {/*x=*/ 0, /*otra=*/ 99, /*y=*/ 1};
	assert(!pre1(s1));
	fig::State s2 = {/*x=*/ 1, /*otra=*/ -9, /*y=*/ 0};
	assert(!pre1(s2));

	const std::string str2("x^y >= max(x,y)");
	const std::set<std::string> varnames2({"x","y"});
	fig::Precondition pre2(str2, varnames2);
	assert(pre2(s2));
	fig::State s3 = {/*x=*/ 3, /*otra=*/ std::numeric_limits<short>::max(),
					 /*y=*/ 9};
	assert(pre2(s3));
	fig::State s4 = {/*x=*/ 2, /*otra=*/ std::numeric_limits<short>::min(),
					 /*y=*/ 16};
	assert(!pre2(s4));  // since MUP_BASETYPE is short, 2^16 should overflow

	// Incorrect creation data
	const std::string str3("x-y-z < _pi^2");
	const std::list<std::string> varnames3({"x","y"});  // forgot "z"
	try {
		fig::Precondition pre3(str3, varnames3);  // should throw due to unexpected "z"
		throw TestException(to_string(__LINE__).append(": previous statement "
													   "should have thrown"));
	} catch (fig::FigException) { /* this was expected */ }
}


static void // ////////////////////////////////////////////////////////////////
//
test_postcondition()
{
	const std::string str1("2*y , x^_pi");  // 2 updates
	const std::set<std::string> varNames1({"x","y"});
	const std::list<std::string> varUpdates1({"x","y"});  // apply updates to 'x' and 'y' resp.
	fig::Postcondition pos1(str1, varNames1, varUpdates1);

	// Positions of variables in State instances are determined by the
	// unique GlobalState 'gState' object
	fig::State s1 = {/*x=*/ 0, /*otra=*/ 99, /*y=*/ 1};
	pos1(s1);
	assert(2 == s1[0] && 0 == s1[2]);
	fig::Postcondition pos2(str1,
							varNames1.begin(), varNames1.end(),
							varUpdates1.begin(), varUpdates1.end());
	fig::State s2 = {/*x=*/ 0, /*otra=*/ 99, /*y=*/ 1};
	assert(pos2.expression() == pos1.expression());
	pos2(s2);
	assert(s1 == s2);
	fig::Postcondition pos3(pos2);
	pos2(s1);
	pos3(s2);
	assert(s1 == s2);

	// Incorrect creation data
	const std::string str4("x-y-z, _pi^2");
	const std::list<std::string> varNames4({"x","y"});  // forgot "z"
	const std::list<std::string> varUpdates4({"x","y"});  // apply updates to 'x' and 'y' resp.
	static_assert(std::is_same<decltype(varUpdates4), const std::list<std::string>>::value, "ERROR");
	try {
		fig::Precondition pos4(str4, varNames4, varUpdates4);  // should throw due to unexpected "z"
		throw TestException(to_string(__LINE__).append(": previous statement "
													   "should have thrown"));
	} catch (fig::FigException) { /* this was expected */ }

	/// @todo: TODO Complete this test
//	fig::Postcondition pos1(pos1str, 2, varnames1);
//	assert(pos1str == pos1.expression());
}


static void // ////////////////////////////////////////////////////////////////
//
test_transition()
{
	/// @todo: TODO Complete this test
}
