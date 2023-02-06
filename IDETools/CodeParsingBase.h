#ifndef CodeParsingBase_h
#define CodeParsingBase_h
#pragma once


namespace str
{
	template< typename CharT, typename PosT >
	inline bool IsValidPos( PosT pos, const CharT* pText ) { return pos >= 0 && pos < static_cast<PosT>( str::GetLength( pText ) ); }
}


namespace code
{
	bool IsBrace( wchar_t brace );				// works for both char/wchar_t
	bool IsOpenBrace( wchar_t brace );
	bool IsCloseBrace( wchar_t brace );

	wchar_t _GetMatchingBrace( wchar_t brace );

	template< typename CharT >
	inline CharT GetMatchingBrace( CharT brace ) { return static_cast<CharT>( _GetMatchingBrace() ); }
}


#endif // CodeParsingBase_h
