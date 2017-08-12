# pragma once
// 从boost/any.h移植，用标准库替代实现
#include <algorithm>
#include <typeinfo>

namespace VK {
	// template class disable_if
	template <bool Condition,
	          class T = void>
	struct disable_if { // type is undefined for assumed !Condition
	};

	template <typename T>
	struct disable_if<false, T> {
		typedef T type;
	};

	// template class add_reference
	template <typename T>
	struct add_reference {
		typedef typename std::conditional<std::is_reference<T>::value, T, T&>::type type;
	};

	class any {
	public: // structors
		any()
			: content(nullptr) { }

		template <typename ValueType>
		explicit any(const ValueType& value)
			: content(new holder<
				typename std::remove_cv<typename std::decay<const ValueType>::type>::type>(value)) { }

		any(const any& other)
			: content(other.content ? other.content->clone() : nullptr) { }

		// Move constructor
		any(any&& other)
			: content(other.content) {
			other.content = nullptr;
		}

		// Perfect forwarding of ValueType
		template <typename ValueType>
		explicit any(ValueType&& value
		             , typename disable_if<std::is_same<any&, ValueType>::value>::type* = nullptr // disable if value has type `any&`
		             , typename disable_if<std::is_const<ValueType>::value>::type* = nullptr) // disable if value has type `const ValueType&&`
			: content(new holder<typename std::decay<ValueType>::type>(static_cast<ValueType&&>(value))) { }

		~any() {
			delete content;
		}

		// modifiers

		any& swap(any& rhs) {
			std::swap(content, rhs.content);
			return *this;
		}

		any& operator=(const any& rhs) {
			any(rhs).swap(*this);
			return *this;
		}

		// move assignement
		any& operator=(any&& rhs) {
			rhs.swap(*this);
			any().swap(rhs);
			return *this;
		}

		// Perfect forwarding of ValueType
		template <class ValueType>
		any& operator=(ValueType&& rhs) {
			any(static_cast<ValueType&&>(rhs)).swap(*this);
			return *this;
		}

		// queries
		bool empty() const {
			return !content;
		}

		void clear() {
			any().swap(*this);
		}

		const type_info& type() const {
			return content ? content->type() : typeid(void);
		}

	private: // types
		class placeholder {
		public: // structors
			virtual ~placeholder() { }

			// queries
			virtual const type_info& type() const = 0;
			virtual placeholder* clone() const = 0;
		};

		template <typename ValueType>
		class holder : public placeholder {
		public: // structors
			explicit holder(const ValueType& value)
				: held(value) { }

			explicit holder(ValueType&& value)
				: held(static_cast<ValueType&&>(value)) { }

			// queries
			const type_info& type() const override {
				return typeid(ValueType);
			}

			placeholder* clone() const override {
				return new holder(held);
			}

			// representation
			ValueType held;

		private: // intentionally left unimplemented
			holder& operator=(const holder&) = delete;
		};
		
		// representation
		template <typename ValueType>
		friend ValueType* any_cast(any*);

		placeholder* content;
	};

	inline void swap(any& lhs, any& rhs) {
		lhs.swap(rhs);
	}

	class bad_any_cast : public std::bad_cast {
	public:
		const char* what() const override {
			return "boost::bad_any_cast: "
					"failed conversion using boost::any_cast";
		}
	};

	template <typename ValueType>
	ValueType* any_cast(any* operand) {
		return operand && operand->type() == typeid(ValueType)
			       ? &static_cast<any::holder<typename std::remove_cv<ValueType>::type> *>(operand->content)->held
			       : 0;
	}

	template <typename ValueType>
	const ValueType* any_cast(const any* operand) {
		return any_cast<ValueType>(const_cast<any *>(operand));
	}

	template <typename ValueType>
	ValueType any_cast(any& operand) {
		typedef typename std::remove_reference<ValueType>::type nonref;

		nonref* result = any_cast<nonref>(&operand);
		if (!result)
			throw std::exception(bad_any_cast());

		// Attempt to avoid construction of a temporary object in cases when 
		// `ValueType` is not a reference. Example:
		// `static_cast<std::string>(*result);` 
		// which is equal to `std::string(*result);`
		typedef typename std::conditional<std::is_reference<ValueType>::value,
		                                  ValueType,
		                                  typename add_reference<ValueType>::type>::type ref_type;

		return static_cast<ref_type>(*result);
	}

	template <typename ValueType>
	ValueType any_cast(const any& operand) {
		typedef typename std::remove_reference<ValueType>::type nonref;
		return any_cast<const nonref &>(const_cast<any &>(operand));
	}

	template <typename ValueType>
	ValueType any_cast(any&& operand) {
		static_assert(
			std::is_rvalue_reference<ValueType&&>::value /*true if ValueType is rvalue or just a value*/
			|| std::is_const<typename std::remove_reference<ValueType>::type>::value,
			"boost::any_cast shall not be used for getting nonconst references to temporary objects"
		);
		return any_cast<ValueType>(operand);
	}
}
