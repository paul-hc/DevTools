#ifndef StdHashValue_h
#define StdHashValue_h
#pragma once


// hash function portable across different STL library versions
//
#if _MSC_VER > 1500		// MSVC++ 10.0 + (VStudio2010+)
	#include <xstddef>

	namespace utl
	{
		inline size_t HashValue( const void* pFirst, size_t count )
		{
			return std::_Hash_seq( static_cast<const BYTE*>( pFirst ), count );
		}
	}

#else					// MSVC++ 9.0 (VStudio2009)
	#include <xhash>

	namespace utl
	{
		inline size_t HashValue( const void* pFirst, size_t count )
		{
			const BYTE* pFirstByte = static_cast<const BYTE*>( pFirst );
			return stdext::_Hash_value( pFirstByte, pFirstByte + count );
		}
	}

#endif //_MSC_VER


namespace utl
{
	template< typename T >
	void hash_combine( size_t& rSeed, const T& value )
	{
		rSeed ^= stdext::hash_value( value ) + 0x9e3779b9 + ( rSeed << 6 ) + ( rSeed >> 2 );
	}
}

namespace stdext
{
	template< typename T, typename U >
	inline size_t hash_value( const std::pair< T, U >& p )
	{
		size_t value = stdext::hash_value( p.first );
		utl::hash_combine( value, p.second );
		return value;
	}
}


#endif // StdHashValue_h
