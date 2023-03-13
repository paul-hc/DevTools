#ifndef CppVer_h
#define CppVer_h
#pragma once


/*
	Can't rely on __cplusplus since Visual C++ compiler uses the old 199711L value to prevent breaking legacy code.
	So we are using _MSVC_LANG instead.

	https://stackoverflow.com/questions/62916404/how-to-correctly-set-msvc-lang-value
*/

#if _MSVC_LANG >= 201103L
	#define IS_CPP_11
#endif

#if _MSVC_LANG >= 201402L
	#define IS_CPP_14
#endif

#if _MSVC_LANG >= 201703L
	#define IS_CPP_17
#endif

#if _MSVC_LANG >= 202002L
	#define IS_CPP_20
#endif


/*	https://github.com/AnthonyCalandra/modern-cpp-features

C++11 new language features:
===========================
    move semantics
    variadic templates
    rvalue references
    forwarding references
    initializer lists
    static assertions
    auto
    lambda expressions
    decltype
    type aliases
    nullptr
    strongly-typed enums
    attributes
    constexpr
    delegating constructors
    user-defined literals
    explicit virtual overrides
    final specifier
    default functions
    deleted functions
    range-based for loops
    special member functions for move semantics
    converting constructors
    explicit conversion functions
    inline-namespaces
    non-static data member initializers
    right angle brackets
    ref-qualified member functions
    trailing return types
    noexcept specifier
    char32_t and char16_t
    raw string literals

	C++11 new library features:
		std::move
		std::forward
		std::thread
		std::to_string
		type traits
		smart pointers
		std::chrono
		tuples
		std::tie
		std::array
		unordered containers
		std::make_shared
		std::ref
		memory model
		std::async
		std::begin/end


C++14 new language features:
===========================
	binary literals
	generic lambda expressions
	lambda capture initializers
	return type deduction
	decltype(auto)
	relaxing constraints on constexpr functions
	variable templates
	[[deprecated]] attribute

	C++14 new library features:
		user-defined literals for standard library types
		compile-time integer sequences
		std::make_unique


C++17 new language features:
===========================
	template argument deduction for class templates
	declaring non-type template parameters with auto
	folding expressions
	new rules for auto deduction from braced-init-list
	constexpr lambda
	lambda capture this by value
	inline variables
	nested namespaces
	structured bindings
	selection statements with initializer
	constexpr if
	utf-8 character literals
	direct-list-initialization of enums
	[[fallthrough]], [[nodiscard]], [[maybe_unused]] attributes
	__has_include
	class template argument deduction

	C++17 new library features:
		std::variant
		std::optional
		std::any
		std::string_view
		std::invoke
		std::apply
		std::filesystem
		std::byte
		splicing for maps and sets
		parallel algorithms
		std::sample
		std::clamp
		std::reduce
		prefix sum algorithms
		gcd and lcm
		std::not_fn
		string conversion to/from numbers


C++20 new language features:
===========================
	coroutines
	concepts
	designated initializers
	template syntax for lambdas
	range-based for loop with initializer
	[[likely]] and [[unlikely]] attributes
	deprecate implicit capture of this
	class types in non-type template parameters
	constexpr virtual functions
	explicit(bool)
	immediate functions
	using enum
	lambda capture of parameter pack
	char8_t
	constinit

	C++20 new library features:
		concepts library
		synchronized buffered outputstream
		std::span
		bit operations
		math constants
		std::is_constant_evaluated
		std::make_shared supports arrays
		starts_with and ends_with on strings
		check if associative container has element
		std::bit_cast
		std::midpoint
		std::to_array
*/


#endif // CppVer_h
