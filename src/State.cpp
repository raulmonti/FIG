#include <utility>  // std::move(), std::swap()
#include <cassert>
#include <State.h>


///////////////////////////////////////////////////////////////////////////////
//
//   Variable
//
////////////////////////////////////////

template< typename T_ >
Variable< T_ >::Variable() : name_(), val_(), min_(), max_()
{ /* empty name indicates fresh variable */ }

template< typename T_ >
Variable< T_ >::Variable(const VarDec< T_ >& dec) :
	name_(std::get<0>(dec)),
	val_(std::get<1>(dec)),
	min_(std::get<1>(dec)),
	max_(std::get<2>(dec))
{
	assert_invariant();
}

template< typename T_ >
Variable< T_ >::Variable(const VarDef< T_ >& def) :
	name_(std::get<0>(def)),
	val_(std::get<3>(def)),
	min_(std::get<1>(def)),
	max_(std::get<2>(def))
{
	assert_invariant();
}

template< typename T_ >
Variable< T_ >::Variable(VarDec< T_ >&& dec) :
	name_(std::move(std::get<0>(dec))),
	val_(std::move(std::get<1>(dec))),
	min_(std::move(std::get<1>(dec))),
	max_(std::move(std::get<2>(dec)))
{
	assert_invariant();
}

template< typename T_ >
Variable< T_ >::Variable(VarDef< T_ >&& def) :
	name_(std::move(std::get<0>(def))),
	val_(std::move(std::get<3>(def))),
	min_(std::move(std::get<1>(def))),
	max_(std::move(std::get<2>(def)))
{
	assert_invariant();
}

template< typename T_ >
Variable< T_ >::Variable(const string& name, T_ min, T_ max) :
	name_(name),
	val_(min),
	min_(min),
	max_(max)
{
	assert_invariant();
}

template< typename T_ >
Variable< T_ >::Variable(const string& name, T_ val, T_ min, T_ max) :
	name_(name),
	val_(val),
	min_(min),
	max_(max)
{
	assert_invariant();
}

template< typename T_ >
Variable< T_ >::Variable(string&& name, T_ min, T_ max) :
	name_(std::move(name)),
	val_(std::move(min)),
	min_(std::move(min)),
	max_(std::move(max))
{
	assert_invariant();
}

template< typename T_ >
Variable< T_ >::Variable(string&& name, T_ val, T_ min, T_ max) :
	name_(std::move(name)),
	val_(std::move(val)),
	min_(std::move(min)),
	max_(std::move(max))
{
	assert_invariant();
}

template< typename T_ >
Variable< T_ >& Variable< T_ >::operator=(Variable< T_ > that)
{
	assert(name_.empty());  // can only assign to a fresh variable
	name_ = std::move(that.name_);
	val_  = std::move(that.val_);
	min_  = std::move(that.min_);
	max_  = std::move(that.max_);
	return *this;
}

template< typename T_ >
Variable< T_ >& Variable< T_ >::operator=(VarDec< T_ > dec)
{
	assert(name_.empty());  // can only assign to a fresh variable
	name_ = std::move(std::get<0>(dec));
	val_  = std::move(std::get<1>(dec));
	min_  = std::move(std::get<1>(dec));
	max_  = std::move(std::get<2>(dec));
	return *this;
}

template< typename T_ >
Variable< T_ >& Variable< T_ >::operator=(VarDef< T_ > def)
{
	assert(name_.empty());  // can only assign to a fresh variable
	name_ = std::move(std::get<0>(def));
	val_  = std::move(std::get<3>(def));
	min_  = std::move(std::get<1>(def));
	max_  = std::move(std::get<2>(def));
	return *this;
}


// Variable can only be instantiated with following numeric types
template class Variable< short              >;
template class Variable< int                >;
template class Variable< long               >;
template class Variable< long long          >;
template class Variable< unsigned short     >;
template class Variable< unsigned int       >;
template class Variable< unsigned long      >;
template class Variable< unsigned long long >;



///////////////////////////////////////////////////////////////////////////////
//
//   State
//
////////////////////////////////////////

template< typename T_ >
State< T_ >::State() : vars_p(nullptr) {}

template< typename T_ >
State< T_ >::State(const vector< VarDec< T_ > >& vars) :
	vars_p(std::make_shared< vector< Variable< T_ > > >(vars.size()))
{
	for (size_t i=0 ; i < vars.size() ; i++)
		(*vars_p)[i] = vars[i];
}

template< typename T_ >
State< T_ >::State(const vector< VarDef< T_ > >& vars) :
	vars_p(std::make_shared< vector< Variable< T_ > > >(vars.size()))
{
	for (size_t i=0 ; i < vars.size() ; i++)
		(*vars_p)[i] = vars[i];
}

template< typename T_ >
State< T_ >::State(const State< T_ >& that) noexcept
{
	if (0 == that.size())
		vars_p = nullptr;
	else {
		vars_p = std::make_shared< vector< Variable< T_ > > >();
		(*vars_p) = (*that.vars_p);  // copy vector whole
	}
}

template< typename T_ >
State< T_ >& State< T_ >::operator=(State< T_ > that) noexcept
{
	std::swap(vars_p, that.vars_p);
	return *this;
}

template< typename T_ >
bool State< T_ >::compatible(const State< T_ >& that) const
{
	if (this->size() != that.size())
		return false;
	#pragma omp parallel for
	for (size_t i=0 ; i < vars_p->size() ; i++)
		if ( ! that[i].compatible((*vars_p)[i]) )
			return false;
	return true;
}

template< typename T_ >
bool State< T_ >::operator==(const State< T_ >& that) const
{
	if (this->size() != that.size())
		return false;
	#pragma omp parallel for
	for (size_t i=0 ; i < vars_p->size() ; i++)
		if ( that[i] != (*vars_p)[i] )
			return false;
	return true;
}


// State can only be instantiated with following numeric types
template class State< short              >;
template class State< int                >;
template class State< long               >;
template class State< long long          >;
template class State< unsigned short     >;
template class State< unsigned int       >;
template class State< unsigned long      >;
template class State< unsigned long long >;

