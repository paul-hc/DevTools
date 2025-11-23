
#include "pch.h"
#include "StdHashValue.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace utl
{
	size_t HashBytes( const void* pFirst, size_t count )
	{
		REQUIRE( count != utl::npos );

		const BYTE* pBytes = static_cast<const BYTE*>( pFirst );

		// inspired from template class instantiation 'template<> class hash<std::string>' from <functional> - hashing mechanism for std::unordered_map, std::unordered_set, etc.
		size_t hashValue = utl::HashSeed;
		size_t pos = 0;
		size_t lastPos = count;
		size_t stridePos = 1 + lastPos / 10;

		if ( stridePos < lastPos )
			lastPos -= stridePos;

		for ( ; pos < lastPos; pos += stridePos )
			hashValue = utl::HashPrime * hashValue ^ static_cast<size_t>( pBytes[ pos ] );

		return hashValue;
	}


	// compute hash value of an integral value type based on a value conversion functor - e.g. for case-insensitive string hashing:
	//	Obsolete: use HashArray() instead. This is just for illustration.
	//
	template< typename ValueT, typename ToValueFuncT >
	size_t HashArray_vc9( const ValueT* pFirst, size_t count, ToValueFuncT toValueFuncT )
	{
		// inspired from template class instantiation 'template<> class hash<std::wstring>' from <functional> in VC9 - hashing mechanism for std::unordered_map, std::unordered_set, etc.
		size_t hashValue = utl::HashSeed;
		size_t pos = 0;
		size_t lastPos = count;
		size_t stridePos = 1 + lastPos / 10;

		if ( stridePos < lastPos )
			lastPos -= stridePos;

		for ( ; pos < lastPos; pos += stridePos )
			hashValue = utl::HashPrime * hashValue ^ static_cast<size_t>( toValueFuncT( pFirst[ pos ] ) );

		return hashValue;
	}

	void f()
	{
		{
			const char* pString = "Some string";
			size_t hashA = HashArray_vc9( pString, str::GetLength( pString ), func::ToLower() ); hashA;
		}
		{
			const wchar_t* pString = L"Some string";
			size_t hashW = HashArray_vc9( pString, str::GetLength( pString ), func::ToLower() ); hashW;
		}
	}
}
