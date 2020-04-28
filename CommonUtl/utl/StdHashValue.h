#ifndef StdHashValue_h
#define StdHashValue_h
#pragma once

#include <xhash>


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
