
#ifndef MOM_SLOT_H
#define MOM_SLOT_H

// lizs 2017.3.4
// ²Î¿¼
// http://marcmo.github.io/delegates/

namespace VK {
	/**
	* non specialized template declaration for slot
	*/
	template <typename T>
	class slot;

	/**
	* specialization for member functions
	*
	* \tparam T            class-type of the object who's member function to call
	* \tparam R            return type of the function that gets captured
	* \tparam params       variadic template list for possible arguments
	*                      of the captured function
	*/
	template <typename T, typename R, typename... Params>
	class slot<R(T::*)(Params ...)> {
	public:
		typedef R (T::*func_type)(Params ...);

		slot(func_type func, T& callee)
			: callee_(callee)
			  , func_(func) {}

		R operator()(Params ... args) const {
			return (callee_ .* func_)(args ...);
		}

		bool operator==(const slot& other) const {
			return (&callee_ == &other.callee_) && (func_ == other.func_);
		}

		bool operator!=(const slot& other) const {
			return !((*this) == other);
		}

	private:
		T& callee_;
		func_type func_;
	};

	/**
	* specialization for const member functions
	*/
	template <typename T, typename R, typename... Params>
	class slot<R(T::*)(Params ...) const> {
	public:
		typedef R (T::*func_type)(Params ...) const;

		slot(func_type func, const T& callee)
			: callee_(callee)
			  , func_(func) {}

		R operator()(Params ... args) const {
			return (callee_ .* func_)(args ...);
		}

		bool operator==(const slot& other) const {
			return (&callee_ == &other.callee_) && (func_ == other.func_);
		}

		bool operator!=(const slot& other) const {
			return !(*this == other);
		}

	private:
		const T& callee_;
		func_type func_;
	};

	/**
	* specialization for free functions
	*
	* \tparam R            return type of the function that gets captured
	* \tparam params       variadic template list for possible arguments
	*                      of the captured function
	*/
	template <typename R, typename... Params>
	class slot<R(*)(Params ...)> {
	public:
		typedef R (*func_type)(Params ...);

		explicit slot(func_type func)
			: func_(func) {}

		R operator()(Params ... args) const {
			return (*func_)(args ...);
		}

		bool operator==(const slot& other) const {
			return func_ == other.func_;
		}

		bool operator!=(const slot& other) const {
			return !((*this) == other);
		}

	private:
		func_type func_;
	};

	/**
	* function to deduce template parameters from call-context
	*/
	template <typename F, typename T>
	slot<F> make_slot(F func, T& obj) {
		return slot<F>(func, obj);
	}

	template <typename T>
	slot<T> make_slot(T func) {
		return slot<T>(func);
	}
} // namespace slot

#endif