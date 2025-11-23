#ifndef StdHashValue_h
#define StdHashValue_h
#pragma once

#include <type_traits>		// for std::hash<T>()


namespace utl
{
#if defined(IS_CPP_11)
	_INLINE_VAR constexpr size_t HashSeed = std::_FNV_offset_basis;		// 32-bit: 2166136261u	64-bit: 14695981039346656037ull
	_INLINE_VAR constexpr size_t HashPrime = std::_FNV_prime;			// 32-bit: 16777619u	64-bit: 1099511628211ull;
#else
	// use FNV constants similar to VC17 STL - from Fowler–Noll–Vo hashing algorithms:
	#if defined(_WIN64)
		// 64-bit:
		__declspec(selectany) extern const size_t HashSeed = 14695981039346656037ull;
		__declspec(selectany) extern const size_t HashPrime = 1099511628211ull;
	#else
		// 32-bit:
		__declspec(selectany) extern const size_t HashSeed = 2166136261u;
		__declspec(selectany) extern const size_t HashPrime = 16777619u;
	#endif // _WIN64
#endif

	// hash value for objects (struct, class):

	size_t HashBytes( const void* pFirst, size_t count );


	// hash value for arrays of integral types (string, path, etc):

	template< typename ValueT, typename ToValueFuncT >
	size_t HashAppendBytes( size_t hashValue, const ValueT* pFirst, size_t count, ToValueFuncT toValueFuncT )
	{
		// accumulate range [pFirst, pFirst + count) into partial FNV-1a hashValue - inspired by std::_Fnv1a_append_bytes() in VC17
		for ( size_t i = 0; i != count; ++i )
		{
			hashValue ^= static_cast<size_t>( toValueFuncT( pFirst[ i ] ) );	// value translation
			hashValue *= utl::HashPrime;
		}

		return hashValue;
	}


	template< typename ValueT, typename ToValueFuncT >
	size_t HashArray( const ValueT* pFirst, size_t count, ToValueFuncT toValueFuncT )
	{	// bitwise hashes the representation of an array - inspired by std::_Hash_array_representation() in VC17
		return HashAppendBytes( utl::HashSeed, pFirst, count, toValueFuncT );	// value translation
	}
}


namespace utl
{
	template< typename T >
	inline size_t GetHashValue( const T& value )
	{
		return std::hash<T>()( value );
	}


	inline void HashCombine( size_t* pSeed, size_t hashValue )
	{
		*pSeed ^= hashValue + 0x9e3779b9 + ( *pSeed << 6 ) + ( *pSeed >> 2 );
	}

	template< typename T1, typename T2 >
	inline size_t GetHashCombine( const T1& value1, const T2& value2 )
	{
		size_t hashValue = std::hash<T1>()( value1 );

		HashCombine( &hashValue, std::hash<T2>()( value2 ) );
		return hashValue;
	}

	template< typename T, typename U >
	inline size_t GetPairHashValue( const std::pair<T, U>& p ) { return GetHashCombine( p.first, p.second ); }


	struct CPairHasher		// use with std::unordered_map<> for pairs
	{
		template< typename T, typename U >
		inline size_t operator()( const std::pair<T, U>& p ) const
		{
			return GetPairHashValue( p );
		}
	};
}


#endif // StdHashValue_h
