
#include "stdafx.h"
#include "StringCompare.h"
#include "StringIntuitiveCompare.h"


namespace str
{
	// case insensitive intuitive comparison - sequences of digits are compared numerically

	pred::CompareResult IntuitiveCompare( const char* pLeft, const char* pRight )
	{
		return pred::MakeIntuitiveComparator( func::ToChar() ).Compare( pLeft, pRight );
	}

	pred::CompareResult IntuitiveCompare( const wchar_t* pLeft, const wchar_t* pRight )
	{
		return pred::MakeIntuitiveComparator( func::ToChar() ).Compare( pLeft, pRight );
	}

} // namespace str
