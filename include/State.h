#include <tuple>
#include <vector>
#include <memory>     // std::shared_ptr
#include <string>
#include <algorithm>  // std::find()
#include <cassert>


#ifndef _STATE_H
#define _STATE_H

using std::unique_ptr;
using std::shared_ptr;
using std::weak_ptr;
using std::string;
using std::vector;
using std::pair;



// Variable declaration: name, min, max
template< typename T_ > using VarDec = std::tuple< string, T_, T_ >;

// Variable definition: name, min, max, initial value
template< typename T_ > using VarDef = std::tuple< string, T_, T_, T_ >;



/// Integral typed Variable defined by an interval
template< typename T_ >
class Variable
{
	// Friend template class: http://stackoverflow.com/a/8967610
	template< typename TT_ > friend class State;

	static_assert(std::is_integral<T_>::value,
	              "ERROR: class Variable<T> can only be instantiated "
	              "with integral types, e.g. int, short, unsigned.");
	string name_;
	T_ offset_;
	T_ min_;
	T_ max_;

protected:
	inline virtual void assert_invariant()
		{
			assert(!name_.empty());  // false for fresh variables
			assert(0 <= offset_);
			assert(min_ + offset_ <= max_);
		}

public:  // Constructors XXX

	// It's safe to use compiler's move and copy ctors
	// Variable(const Variable& that);
	// Variable(Variable&& that);

	// A bunch of data ctors
	Variable(const VarDec< T_ >& dec);
	Variable(const VarDef< T_ >& def);
	Variable(VarDec< T_ >&& dec);
	Variable(VarDef< T_ >&& def);
	Variable(const string& name, T_ min, T_ max);
	Variable(const string& name, T_ val, T_ min, T_ max);
	Variable(string&& name, T_ min, T_ max);
	Variable(string&& name, T_ val, T_ min, T_ max);

	// Empty ctor creates a "fresh Variable"
	Variable();

	// Full copy/move assignment can only be done to fresh Variables
	Variable< T_ >& operator=(Variable< T_ > that);
	Variable< T_ >& operator=(VarDec< T_ > dec);
	Variable< T_ >& operator=(VarDef< T_ > def);

	// Only internal value assignment can be done to any Variable
	inline Variable< T_ >& operator=(const T_& val)
		{
			assert(val >= min_);
			assert(val <= max_);
			offset_ = val - min_;
			return *this;
		}

	inline Variable< T_ >& operator=(T_&& val)
		{
			assert(val >= min_);
			assert(val <= max_);
			offset_ = val - min_;
			return *this;
		}

public:  // Accessors XXX

	inline const string& name() const noexcept { return name_; }
	inline T_ val() const noexcept { return min_ + offset_; }
	inline T_ min() const noexcept { return min_; }
	inline T_ max() const noexcept { return max_; }

protected:
	inline T_ range()  const noexcept { return max_-min_+1; }  // closed interval
	inline T_ offset() const noexcept { return offset_; }

public:  // Relational operators XXX

	inline bool compatible(const Variable< T_ >& that) const
		{
			return (min_ == that.min_ &&
			        max_ == that.max_ &&
			        name_ == that.name_);
		}

	inline bool operator==(const Variable< T_ >& that) const
		{
			return (compatible(that) && offset_ == that.offset_);
		}

	inline bool operator!=(const Variable< T_ >& that) const
		{
			return (!compatible(that) || offset_ != that.offset_);
		}
};



/// Symbolic representation of a state, viz. a vector of Variables
template< typename T_ >
class State
{
	static_assert(std::is_integral<T_>::value,
	              "ERROR: class State<T> can only be instantiated "
	              "with integral types, e.g. int, short, unsigned.");

protected:

	shared_ptr< vector< Variable< T_ > > > vars_p;

public:  // Constructors XXX

	// Void ctor
	State();
	// Data ctor
	State(const vector< VarDec< T_ > >& vars);
	State(const vector< VarDef< T_ > >& vars);
	// Copy ctor
	State(const State< T_ >& that) noexcept;
	// Move ctor
	State(State< T_ >&& that) noexcept = default;
	// Copy assignment with copy&swap (no need for move assignment)
	State< T_ >& operator=(State< T_ > that) noexcept;
	// Dtor
	virtual ~State() {}

public:  // Accessors XXX

	inline unsigned size() const
		{ return nullptr == vars_p ? 0 : vars_p->size(); }

	// @brief Retrieve i-th variable as const reference
	// @compl Constant
	inline const Variable< T_ >& operator[](size_t i) const
		{
			assert(nullptr != vars_p);
#		ifndef NDEBUG
			return const_cast<const Variable< T_ >& >(vars_p->at(i));
#		else
			return (*vars_p)[i];
#		endif
		}

	// @brief Retrieve i-th variable
	// @compl Constant
	inline Variable< T_ >& operator[](size_t i)
		{
			assert(nullptr != vars_p);
#		ifndef NDEBUG
			return vars_p->at(i);
#		else
			return (*vars_p)[i];
#		endif
		}

	// @brief Retrieve variable named "varname" if existent
	// @compl Linear on the size of State
	inline shared_ptr< Variable< T_ > > operator[](const string& varname)
		{
			for (auto& e: *vars_p)
				if (varname == e.name())
					return std::make_shared< Variable< T_ >>(e);
			return nullptr;
		}

	// @brief Encode current state (viz. vector of Variables values) as a number,
	//        i.e. the "concrete" representation of the current state.
	// @compl Quadratic on the size of State
	size_t encode_state() const;

	// @brief Decode number into vector of variable values and apply to State,
	//        i.e. store "symbolically" the "concrete state" n.
	// @compl Quadratic on the size of State
	void decode_state(const size_t& n);
	
public:  // Relational operators XXX

	// @brief Tell whether 'this' and 'that' hold compatible/same variables
	// @compl Linear on the size of State
	bool compatible(const State< T_ >& that) const;
	bool operator==(const State< T_ >& that) const;
	inline bool operator!=(const State< T_ >& that) const
		{ return ( ! (that == *this) ); }
};

#endif

