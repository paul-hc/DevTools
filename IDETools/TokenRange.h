#ifndef TokenRange_h
#define TokenRange_h
#pragma once

#include "utl/Range.h"


namespace str
{
	template< typename PosT, typename StringT >
	inline bool IsValidPos( PosT pos, const StringT& codeText ) { return static_cast<size_t>( pos ) < codeText.length(); }
}


struct TokenRange : public Range<int>
{
	typedef Range<int> TRange;
public:
	TokenRange( int startAndEnd = 0 ) : TRange( startAndEnd ) {}
	TokenRange( int start, int end ) : TRange( start, end ) {}
	TokenRange( const TCHAR* pText, int start = 0 ) { setString( pText, start ); }

	template< typename PosT >
	explicit TokenRange( const Range<PosT>& range ) : TRange( range ) {}

	bool operator==( const TokenRange& right ) const
	{
		return
			&right == this ||
			( IsEmpty() && right.IsEmpty() ) ||
			( m_start == right.m_start && m_end == right.m_end );
	}

	bool operator!=( const TokenRange& right ) const { return !operator==( right ); }
	bool operator<( const TokenRange& right ) const { return &right != this && m_start < right.m_start; }

	bool IsValid( void ) const { return m_start >= 0 && m_end >= 0; }
	bool InStringBounds( const TCHAR* pText ) const;

	int getLength( void ) const { ASSERT( IsValid() ); return m_end - m_start; }
	void setLength( int length ) { ASSERT( m_start >= 0 ); m_end = m_start + length; }

	void assign( int start, int end ) { m_start = start; m_end = end; }
	void setString( const TCHAR* pText, int start = 0 );
	void setEmpty( int startAndEnd ) { m_start = m_end = startAndEnd; }
	void setWithLength( int start, int length ) { m_start = start; setLength( length ); }

	static TokenRange endOfString( const TCHAR* pText ) { return TokenRange( (int)str::GetLength( pText ) ); }

	void gotoEnd( const TCHAR* pText ) { ASSERT_PTR( pText ); setEmpty( (int)str::GetLength( pText ) ); }

	void extendToEnd( const TCHAR* pText ) { ASSERT( pText != NULL && m_start >= 0 ); m_end = (int)str::GetLength( pText ); }

	void incrementBy( int increment );

	void inflateBy( int delta );
	void deflateBy( int delta ) { inflateBy( -delta ); }

	void normalize( void );
	void normalize( const TCHAR* pText );

	// string operations
	std::tstring GetToken( const TCHAR* pText ) const { ASSERT( InStringBounds( pText ) ); return std::tstring( pText + m_start, getLength() ); }
	std::tstring MakeToken( const std::tstring& text ) const { ASSERT( InStringBounds( text.c_str() ) ); return text.substr( m_start, m_end - m_start ); }

	void Trim( const std::tstring& text );

	CString getString( const TCHAR* pText ) const;
	CString getPrefixString( const TCHAR* pText ) const;
	CString getSuffixString( const TCHAR* pText ) const;

	bool isTokenMatch( const TCHAR* pText, const TCHAR* pToken, str::CaseType caseType = str::Case ) const;

	TokenRange& replaceWithToken( CString* pTargetString, const TCHAR* pToken );
	TokenRange& smartReplaceWithToken( CString* pTargetString, const TCHAR* pToken );
};


namespace str
{
	inline std::tstring ExtractString( const TokenRange& tokenRange, const std::tstring& text )
	{
		return text.substr( tokenRange.m_start, tokenRange.m_end - tokenRange.m_start );
	}
}


// inline code

inline void TokenRange::inflateBy( int delta )
{
	ASSERT( IsValid() && IsNormalized() );
	m_start -= delta;
	m_end += delta;
}


#endif // TokenRange_h
