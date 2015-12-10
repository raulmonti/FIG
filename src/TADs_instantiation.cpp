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
#include <sstream>
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
#include <Transition.h>

using std::to_string;
using std::make_tuple;
using std::begin;
using std::end;
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
	inline const std::string& msg() { return msg_; }
};


int main()
{
	std::cout << "\nIgnore ALL following messages BUT the last line.\n"
			  << std::endl;

	try {
		test_label();
		test_clock();
		test_variable_interval();
		test_variable_set();
		test_state();
		test_math_expression();
		test_precondition();
		test_postcondition();
		/// @todo TODO: erase below
		exit(EXIT_SUCCESS);
		test_transition();

	} catch (TestException& e) {
		std::stringstream errMsg;
		errMsg << "\nSome test failed: ";
		errMsg << e.msg();
		errMsg << "\n\nCheck above for failed test.\n";
		std::cout << errMsg.str() << std::endl;
		exit(EXIT_FAILURE);

	} catch (fig::FigException& e) {
		std::stringstream errMsg;
		errMsg << "\nSomething failed unexpectedly: ";
		errMsg << e.msg();
		errMsg << "\n\nCheck above for unexpected error.\n";
		std::cout << errMsg.str() << std::endl;
		exit(EXIT_FAILURE);
	}

	std::cout << "\nAll tests were successfull!\n"
			  << std::endl;

	return 0;
}


static void // ////////////////////////////////////////////////////////////////
//
test_label()
{
	fig::Label tau;
	assert(tau.is_output());
	assert(tau.is_tau());

	fig::Label input("a", false);
	assert(!input.is_tau());
	assert(input.is_input());
	assert(tau != input);

	fig::Label output("a", true);
	assert(!output.is_tau());
	assert(!output.is_input());
	assert(output.is_output());
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
	fig::State<TYPE> gState(vars);
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
	fig::State<TYPE> gState2(vars);
	fig::State<TYPE> gState3(std::move(gState));
	assert(gState  != gState3);
	assert(gState2 == gState3);
}


static void // ////////////////////////////////////////////////////////////////
//
test_math_expression()
{
	// Correct expressions
	const std::string str1("x^y > max(x,y)");
	const std::string str2("y, x^2");
	const std::set<std::string> varnames({"x","y"});
	fig::MathExpression expr1(str1, varnames);
	assert(str1 == expr1.expression());
	fig::MathExpression expr2(str2, varnames);
	assert(str2 == expr2.expression());

	// Incorrect creation data
	try {
		const std::string str("x-y-z < _pi^2");
		const std::set<std::string> varnames({"x","y"});  // forgot "z"
		fig::MathExpression expr(str, varnames);  // shouldn't throw anyway
		assert(str == expr.expression());
	} catch (std::exception) {
		throw TestException(to_string(__LINE__).append(": previous statement "
													   "shouldn't have thrown!"));
	}

	try {
		const std::string str("x+y == _pi-0");
		const std::set<std::string> varnames({"x","y","noexiste"});
		fig::MathExpression expr(str, varnames);
		throw TestException(to_string(__LINE__).append(": previous statement "
													   "should have thrown"));
	} catch (std::out_of_range) { /* this was expected */ }
}


static void // ////////////////////////////////////////////////////////////////
//
test_precondition()
{
	const std::string str1("x^y > max(x,y)");
	const std::set<std::string> varnames1({"x","y"});  // vars appearing in str1
	const fig::PositionsMap varsMap({ {"x",0}, {"y",2}, {"otra",1} });

// typedef fig::VariableInterval< fig::STATE_INTERNAL_TYPE > VarType;
// State< STATE_INTERNAL_TYPE > gState(
// 	std::vector< VarType >{
// 		VarType("x", -1912, -1000),
// 		VarType("otra", 13, 14),
// 		VarType("y", 0, 12)
// 	});

	fig::Precondition pre1(str1, varnames1);
	assert(str1 == pre1.expression());
	pre1.pin_up_vars(varsMap);
	fig::StateInstance s1 = {/*x=*/ 0, /*otra=*/ 99, /*y=*/ 1};
	assert(!pre1(s1));
	fig::StateInstance s2 = {/*x=*/ 1, /*otra=*/ -9, /*y=*/ 0};
	assert(!pre1(s2));

	const std::string str2("x^y >= max(x,y)");
	const std::set<std::string> varnames2({"x","y"});
	fig::Precondition pre2(str2, varnames2);
	pre2.pin_up_vars(varsMap);
	assert(pre2(s2));
	fig::StateInstance s3 = {/*x=*/ 3, /*otra=*/ std::numeric_limits<short>::max(), /*y=*/ 9};
	assert(pre2(s3));
	fig::StateInstance s4 = {/*x=*/ 2, /*otra=*/ std::numeric_limits<short>::min(), /*y=*/ 16};
	assert(!pre2(s4));  // since MUP_BASETYPE is short, 2^16 should overflow

	// Incorrect creation data
	try {
		const std::string str("x-y-z < _pi^2");
		const std::list<std::string> varnames({"x","noexiste","y"});
		fig::Precondition pre(str, varnames);
		throw TestException(to_string(__LINE__).append(": previous statement "
													   "should have thrown"));
	} catch (std::out_of_range) { /* this was expected */ }
	try {
		const std::string str("x-y-z < _pi^2");
		const std::list<std::string> varnames({"x","y","z"});
		fig::StateInstance s(10,0);
		fig::Precondition pre(str, varnames);
		assert(pre(s));  // should throw since the vars weren't pinned
		throw TestException(to_string(__LINE__).append(": previous statement "
													   "should have thrown"));
	} catch (fig::FigException) { /* this was expected */ }
	try {
		const std::string str("x-y-z < _pi^2");
		const std::list<std::string> varnames({"x","y","z"});
		fig::Precondition pre(str, varnames);
		pre.pin_up_vars(fig::PositionsMap({{"x",0}}));  // forgot to map "y" and "z"
		throw TestException(to_string(__LINE__).append(": previous statement "
													   "should have thrown"));
	} catch (std::out_of_range) { /* this was expected */ }
	try {
		const std::string str("x-y-z < _pi^2");
		const std::list<std::string> varnames({"x","y"});  // forgot "z"
		fig::Precondition pre(str, varnames);
		pre.pin_up_vars(fig::PositionsMap({{"x",0},{"y",1}}));
		fig::StateInstance s(10,0);
		pre(s1);  // should throw since "z" wasn't mapped
		throw TestException(to_string(__LINE__).append(": previous statement "
													   "should have thrown"));
	} catch (fig::FigException) { /* this was expected */ }
}


static void // ////////////////////////////////////////////////////////////////
//
test_postcondition()
{
	const std::string str1("2*y , x^_pi");  // 2 updates
	const std::set<std::string> varNames1({"x","y"});  // vars appearing in str1
	const std::list<std::string> varUpdates1({"x","y"});  // apply updates to 'x' and 'y' resp.
	const fig::PositionsMap varsMap({ {"y",2}, {"x",0}, {"otra",1}, {"w",9999} });

	fig::Postcondition pos1(str1, varNames1, varUpdates1);
	assert(str1 == pos1.expression());
	pos1.pin_up_vars(varsMap);
	fig::StateInstance s1 = {/*x=*/ 0, /*otra=*/ 99, /*y=*/ 1};
	auto s2(s1);  // for later
	pos1(s1);
	assert(2 == s1[0]);  // x ==  2*y  ==  2*1  == 2
	assert(0 == s1[2]);  // y == x^_pi == 0^_pi == 0

	fig::Postcondition pos2(str1, varNames1.begin(), varNames1.end(),
								  varUpdates1.begin(), varUpdates1.end());
	assert(pos2.expression() == pos1.expression());
	assert(s1 != s2);
	pos2.pin_up_vars(varsMap);
	pos2(s2);
	assert(s1 == s2);

	fig::Postcondition pos3(pos2);
	pos2(s1);
	pos3(s2);
	assert(s1 == s2);

		/// @todo TODO: erase below
		exit(EXIT_SUCCESS);

	const std::string str4("x^y, 2 - y^(max(x,y))");
	const auto varNames4(varNames1);
	const auto varUpdates4(varUpdates1);  // apply updates to 'x' and 'y' resp.
	fig::Postcondition pos4(str4, varNames4, varUpdates4);
	fig::StateInstance s4 = {/*x*/ 2, /*otra*/ 1115 , /*y*/ 0};
	pos4(s4);
	assert(1 == s4[0]);  // x == x^y == 2^0 == 1
	assert(2 == s4[2]);  // y == 2 - y^max(x,y) == 2 - 0^max(2,0) == 2
	pos4(s4);
	assert(1 == s4[0]);  // x == x^y == 1^2 == 1
	assert(-2 == s4[2]);  // y == 2 - y^max(x,y) == 2 - 2^max(1,2) == -2
	pos4(s4);
	assert(1 == s4[0]);  // x == x^y == 1^-2 == (short)(1/2) == 1
	assert(4 == s4[2]);  // y == 2 - y^max(x,y) == 2 - (-2)^max(1,-2) == 4

	// Incorrect creation data
	try {
		const std::string str("x-y-z, _pi^2");
		const std::list<std::string> varNames({"x","y"});  // forgot "z"
		auto varUpdates(varUpdates1);
		fig::Postcondition pos(str, varNames, varUpdates);  // should throw due to unexpected "z"
		throw TestException(to_string(__LINE__).append(": previous statement "
													   "should have thrown"));
	} catch (fig::FigException) { /* this was expected */ }
	try {
		const std::string str("x-y-z, _pi^2");
		const std::list<std::string> varNames({"x","y","z"});
		const std::list<std::string> varUpdates({"noexiste","z"});
		fig::Postcondition pos(str, varNames, varUpdates);
		throw TestException(to_string(__LINE__).append(": previous statement "
													   "should have thrown"));
	} catch (std::out_of_range) { /* this was expected */ }
}


static void // ////////////////////////////////////////////////////////////////
//
test_transition()
{
	const fig::Label tau;
	fig::Precondition  pre("x<y", std::set<std::string>({"x","y"}));
	fig::Postcondition pos("x+1", std::set<std::string>({"x"}),
								  std::set<std::string>({"x"}));

//	fig::Transition trans(tau, "clockName",  // would fail assertion
//		pre, pos, std::set<std::string>());
	fig::Transition trans1(tau, "", pre, pos, std::set<std::string>());
	assert(tau == trans1.label());
	assert(trans1.triggeringClock().empty());
	assert(static_cast<fig::Bitflag>(0u) == trans1.resetClocks());

	// Populate global clocks vector
	typedef fig::DistributionParameters Params;
	std::vector< std::string > clockNames = {"c1", "c2", "c3"};
	fig::Label input("a");
	fig::Transition trans2(input, "", pre, pos, std::set<std::string>({clockNames[0]}));
	fig::Label output("a", true);
	std::set<std::string> resetClocks3 = { clockNames[2] };
//	fig::Transition trans(output, "",  // would fail assertion
//		pre, pos, std::set<std::string>());
	fig::Transition trans3(output, clockNames[1], pre, pos, resetClocks3);
	assert(static_cast<fig::Bitflag>(0u) != trans3.resetClocks());
	std::vector<std::string> resetClocks4(clockNames);
	fig::Transition trans4(output, clockNames[1], pre, pos, resetClocks4);
	assert(static_cast<fig::Bitflag>(0u) != trans4.resetClocks());

	// Incorrect creation data
	try {
		fig::Transition trans(output, "invalid_clock_name", pre, pos, resetClocks3);
		throw TestException(to_string(__LINE__).append(": previous statement "
													   "should have thrown"));
	} catch (fig::FigException) { /* this was expected */ }
	try {
		fig::Transition trans(tau, "", pre, pos,
							  std::set<std::string>({"c1","invalid_clock_name"}));
		throw TestException(to_string(__LINE__).append(": previous statement "
													   "should have thrown"));
	} catch (std::out_of_range) { /* this was expected */ }

	/// @todo: TODO Complete this test
}
