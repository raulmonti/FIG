#include <iostream>
#include <functional>
#include <cmath>
#include <omp.h>
//#include "/usr/lib/gcc/x86_64-unknown-linux-gnu/5.3.0/include/omp.h"


/****************************************************
 *
 *  Relevant SO entries:
 *    · http://stackoverflow.com/q/2402579
 *    · http://stackoverflow.com/a/12662950
 *
 ****************************************************/


#define  N  (1u<<21)


struct S2
{
	S2(){}

	void a(double& x) const
	{
		if (x > 123456.0f)
			x -= 123450.0f;
		else if (x < -123456.0f)
			x += 123450.0f;
		else if (isinf(x))
			x = 1.0;
		else if (isnan(x))
			x = -1.0;
		else
			x = log(pow(x,2)) + sinh(M_PI/x);
	}

	void b(double& x) const
	{
		if (x > 12345.0f)
			x -= 12340.0f;
		else if (x < -12345.0f)
			x += 12340.0f;
		else if (isinf(x))
			x = 2.0;
		else if (isnan(x))
			x = -2.0;
		else
			x = pow(log(x),3) - cosh(M_PI/x);
	}
};


struct S1
{
	// Pass S2 instance and test case inside loop
	void loop(double x, const S2& s2, bool a) const
	{
		for (size_t i = 0u ; i < N ; i++)
			update(x, s2, a);
		std::cout << "Result: " << x << std::endl;
	}

	// Pass S2 (or any) function through std::function<>
	void loop(double x, std::function<void(double&)> f) const
	{
		for (size_t i = 0u ; i < N ; i++)
			f(x);
		std::cout << "Result: " << x << std::endl;
	}

	// Pass S2 function through C++ NSMF pointer interface
	void loop(double x, const S2& s2, void (S2::*f)(double&) const) const
	{
		for (size_t i = 0u ; i < N ; i++)
			(s2.*f)(x);
		std::cout << "Result: " << x << std::endl;
	}

	// Pass S2 (or any) function using non-type templates
	template< class F >
	void loop(double x, F f, const S2& s2) const
	{
		for (size_t i = 0u ; i < N ; i++)
			(s2.*f)(x);
		std::cout << "Result: " << x << std::endl;
	}

private:
	inline void update(double& x, const S2& s2, bool a) const
	{
		if (a)
			s2.a(x);
		else
			s2.b(x);
	}
};


// Function pointer typedef
typedef void(S2::*fun)(double&) const;
// S2 function template specialization, just to exercise our C++
template<>
void
S1::loop(double x, fun f, const S2& s2) const
{
	std::cerr << "I'm special!  ";
	for (size_t i = 0u ; i < N ; i++)
		(s2.*f)(x);
	std::cout << "Result: " << x << std::endl;
} 


int main()
{
	S1 s1;
	const S2 s2;
	const double x(123.456);

	std::cerr << "Pass S2 instance and test case inside loop...\n";
	double start = omp_get_wtime();
	s1.loop(x, s2, true);
	s1.loop(x, s2, false);
	std::cerr << "Took " << omp_get_wtime()-start << " s" << std::endl;

	std::cerr << "Pass S2 (or any) function through std::function<>...\n";
	start = omp_get_wtime();
	s1.loop(x, std::bind(&S2::a, s2, std::placeholders::_1));
	s1.loop(x, std::bind(&S2::b, s2, std::placeholders::_1));
	std::cerr << "Took " << omp_get_wtime()-start << " s" << std::endl;

	std::cerr << "Pass S2 function through C++ NSMF-pointer...\n";
	start = omp_get_wtime();
	s1.loop(x, s2, &S2::a);
	s1.loop(x, s2, &S2::b);
	std::cerr << "Took " << omp_get_wtime()-start << " s" << std::endl;

	std::cerr << "Pass S2 (or any) function using non-type templates...\n";
	start = omp_get_wtime();
	s1.loop(x, &S2::a, s2);
	s1.loop(x, &S2::b, s2);
	std::cerr << "Took " << omp_get_wtime()-start << " s" << std::endl;

	return 0;
}


// #include <cstdio>
// 
// struct B {
//  int f() { return 1; }
// };
// 
// class A {
// public:
//  int (B::*x)(); // <- declare by saying what class it is a pointer to
// };
// 
// 
// int main() {
//  A a;
//  B b;
//  a.x = &B::f; // use the :: syntax
//  printf("%d\n",(b.*a.x)()); // use together with an object of its class
// }
