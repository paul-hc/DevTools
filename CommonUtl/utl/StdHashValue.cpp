
#include "pch.h"
#include "StdHashValue.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace utl
{
	size_t HashValue( const void* pFirst, size_t count )
	{
		REQUIRE( count != utl::npos );

		const BYTE* pBytes = static_cast<const BYTE*>( pFirst );

		// inspired from template class instantiation 'template<> class hash<std::string>' from <functional> - hashing mechanism for std::unordered_map, std::unordered_set, etc.
		size_t hashValue = 2166136261u;
		size_t pos = 0;
		size_t lastPos = count;
		size_t stridePos = 1 + lastPos / 10;

		if ( stridePos < lastPos )
			lastPos -= stridePos;

		for ( ; pos < lastPos; pos += stridePos )
			hashValue = 16777619u * hashValue ^ static_cast<size_t>( pBytes[ pos ] );

		return hashValue;
	}
}
