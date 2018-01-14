#ifndef TokenRange_h
#define TokenRange_h
#pragma once


struct TokenRange
{
public:
	TokenRange( int startAndEnd = 0 ) : m_start( startAndEnd ), m_end( startAndEnd ) {}
	TokenRange( int start, int end ) : m_start( start ), m_end( end ) {}
	TokenRange( const TCHAR* pText, int start = 0 ) { setString( pText, start ); }

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
	bool IsEmpty( void ) const { return m_start == m_end; }
	bool IsNormalized( void ) const { return m_start <= m_end; }
	bool InStringBounds( const TCHAR* pText ) const;

	int getLength( void ) const { ASSERT( IsValid() ); return m_end - m_start; }
	void setLength( int length );

	void assign( int start, int end ) { m_start = start; m_end = end; }
	void setString( const TCHAR* pText, int start = 0 );
	void setEmpty( int startAndEnd ) { m_start = m_end = startAndEnd; }
	void setWithLength( int start, int length );

	static TokenRange endOfString( const TCHAR* pText ) { return TokenRange( (int)str::GetLength( pText ) ); }

	void gotoEnd( const TCHAR* pText ) { ASSERT_PTR( pText ); setEmpty( (int)str::GetLength( pText ) ); }

	void extendToEnd( const TCHAR* pText );

	void incrementBy( int increment );

	void inflateBy( int delta );
	void deflateBy( int delta ) { inflateBy( -delta ); }

	void normalize( void );
	void normalize( const TCHAR* pText );

	// string operations
	std::tstring GetToken( const TCHAR* pText ) const { ASSERT( InStringBounds( pText ) ); return std::tstring( pText + m_start, getLength() ); }

	CString getString( const TCHAR* pText ) const;
	CString getPrefixString( const TCHAR* pText ) const;
	CString getSuffixString( const TCHAR* pText ) const;

	bool isTokenMatch( const TCHAR* pText, const TCHAR* pToken, str::CaseType caseType = str::Case ) const;

	TokenRange& replaceWithToken( CString& targetString, const TCHAR* pToken );
	TokenRange& smartReplaceWithToken( CString& targetString, const TCHAR* pToken );
public:
	int m_start;
	int m_end;
};


// inline code

inline void TokenRange::setLength( int length )
{
	ASSERT( m_start >= 0 );
	m_end = m_start + length;
}

inline void TokenRange::setWithLength( int start, int length )
{
	m_start = start;
	setLength( length );
}

inline void TokenRange::extendToEnd( const TCHAR* pText )
{
	ASSERT( pText != NULL && m_start >= 0 );
	m_end = (int)str::GetLength( pText );
}

inline void TokenRange::inflateBy( int delta )
{
	ASSERT( IsValid() && IsNormalized() );
	m_start -= delta;
	m_end += delta;
}


#endif // TokenRange_h
