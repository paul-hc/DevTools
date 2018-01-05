// Copyleft 2004 Paul Cocoveanu
//
#include "stdafx.h"
#include "TokenRange.h"
#include "StringUtilitiesEx.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


bool TokenRange::InStringBounds( const TCHAR* pText ) const
{
	ASSERT_PTR( pText );
	return IsValid() && IsNormalized() && m_end <= (int)str::length( pText );
}

void TokenRange::setString( const TCHAR* pText, int startPos /*= 0*/ )
{
	if ( pText != NULL )
	{
		ASSERT( startPos >= 0 && startPos <= (int)str::length( pText ) );
		assign( startPos, (int)str::length( pText ) );
	}
	else
		setEmpty( -1 );
}

void TokenRange::incrementBy( int increment )
{
	ASSERT( IsValid() );
	m_start += increment;
	m_end += increment;
}

void TokenRange::normalize( void )
{
	if ( IsValid() && !IsNormalized() )
		std::swap( m_start, m_end );
}

void TokenRange::normalize( const TCHAR* pText )
{
	if ( m_start >= 0 && m_end == -1 )
		m_end = (int)str::length( pText );
	else
		normalize();
}

// the token string

CString TokenRange::getString( const TCHAR* pText ) const
{
	ASSERT( IsValid() && IsNormalized() );
	ASSERT( InStringBounds( pText ) );

	CString outTokenString;
	int length = getLength();

	TCHAR* buffer = outTokenString.GetBuffer( length );

	_tcsncpy( buffer, pText + m_start, length );
	outTokenString.ReleaseBuffer( length );
	return outTokenString;
}

// return the string precedeeing the token

CString TokenRange::getPrefixString( const TCHAR* pText ) const
{
	TokenRange prefixRange( 0, m_start );
	return prefixRange.getString( pText );
}

// return the string after the token

CString TokenRange::getSuffixString( const TCHAR* pText ) const
{
	TokenRange suffixRange( pText, m_end );
	return suffixRange.getString( pText );
}

// returns true if this points to the specified token

bool TokenRange::isTokenMatch( const TCHAR* pText, const TCHAR* pToken, str::CaseType caseType /*= str::Case*/ ) const
{
	ASSERT( pToken != NULL );

	if ( IsValid() && IsNormalized() )
	{
		int tokenLength = (int)str::length( pToken );

		if ( getLength() == tokenLength )
			if ( caseType == str::Case )
				return 0 == _tcsncmp( pText + m_start, pToken, tokenLength );
			else
				return 0 == _tcsnicmp( pText + m_start, pToken, tokenLength );
	}

	return false;
}

// replace the pointed token with the new one in targetString, and updates m_end.

TokenRange& TokenRange::replaceWithToken( CString& targetString, const TCHAR* pToken )
{
	ASSERT( IsValid() && IsNormalized() );
	ASSERT( InStringBounds( targetString ) && pToken != NULL );

	targetString.Delete( m_start, getLength() );
	targetString.Insert( m_start, pToken );
	m_end = m_start + (int)str::length( pToken );

	return *this;
}

TokenRange& TokenRange::smartReplaceWithToken( CString& targetString, const TCHAR* pToken )
{
	if ( !isTokenMatch( targetString, pToken ) )
		replaceWithToken( targetString, pToken );

	return *this;
}
