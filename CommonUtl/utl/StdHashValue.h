#ifndef StdHashValue_h
#define StdHashValue_h
#pragma once


namespace utl
{
	size_t HashValue( const void* pFirst, size_t count );
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
