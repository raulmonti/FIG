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


template< typename T_ >
class Variable
{
	static_assert(std::is_integral<T_>::value,
	              "ERROR: class Variable<T> can only be instantiated "
	              "with integral types, e.g. int, short, unsigned.");
	string name_;
	T_ val_;
	T_ min_;
	T_ max_;

protected:
	inline virtual void assert_invariant()
		{
			assert(!name_.empty());  // false for fresh variables
			assert(min_ <= max_);
			assert(val_ >= min_);
			assert(val_ <= max_);
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
			val_ = val;
			return *this;
		}

	inline Variable< T_ >& operator=(T_&& val)
		{
			assert(val >= min_);
			assert(val <= max_);
			val_ = std::move(val);
			return *this;
		}

public:  // Accessors XXX

	inline const string& name() const noexcept { return name_; }
	inline T_ val() const noexcept { return val_; }
	inline T_ min() const noexcept { return min_; }
	inline T_ max() const noexcept { return max_; }

public:  // Relational operators XXX

	inline bool compatible(const Variable< T_ >& that) const
		{
			return (min_ == that.min_ &&
			        max_ == that.max_ &&
			        name_ == that.name_);
		}

	inline bool operator==(const Variable< T_ >& that) const
		{
			return (compatible(that) && val_ == that.val_);
		}

	inline bool operator!=(const Variable< T_ >& that) const
		{
			return (val_ != that.val_ || !compatible(that));
		}
};



template< typename T_ >
class State
{
	static_assert(std::is_integral<T_>::value,
	              "ERROR: class State<T> can only be instantiated "
	              "with integral types, e.g. int, short, unsigned.");

//	friend void swap(State< T_ >& left, State< T_ >& right)
//		{ std::swap(left.vars_p, right.vars_p); }

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


	// TODO: {en,de}code_state()
	//
	// Transform (symbolic) State to/from (concrete) numeric value
	// which could be interpreted by the (concrete) importance function.
	
public:  // Relational operators XXX

	// @brief Tell whether 'this' and 'that' hold compatible/same variables
	// @compl Linear on the size of State
	bool compatible(const State< T_ >& that) const;
	bool operator==(const State< T_ >& that) const;
	inline bool operator!=(const State< T_ >& that) const
		{ return ( ! (that == *this) ); }
};

#endif

