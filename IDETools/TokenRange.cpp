// Copyleft 2004 Paul Cocoveanu
//
#include "pch.h"
#include "TokenRange.h"
#include "utl/StringCompare.h"
//#include "StringUtilitiesEx.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


bool TokenRange::InStringBounds( const TCHAR* pText ) const
{
	ASSERT_PTR( pText );
	return IsValid() && IsNormalized() && (size_t)m_end <= str::GetLength( pText );
}

void TokenRange::setString( const TCHAR* pText, int startPos /*= 0*/ )
{
	if ( pText != nullptr )
	{
		ASSERT( (size_t)startPos <= str::GetLength( pText ) );
		assign( startPos, (int)str::GetLength( pText ) );
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

void TokenRange::inflateBy( int delta )
{
	ASSERT( IsValid() && IsNormalized() );
	m_start -= delta;
	m_end += delta;
}

void TokenRange::normalize( void )
{
	if ( IsValid() && !IsNormalized() )
		std::swap( m_start, m_end );
}

void TokenRange::normalize( const TCHAR* pText )
{
	if ( m_start >= 0 && -1 == m_end )
		m_end = (int)str::GetLength( pText );
	else
		normalize();
}

// the token string

void TokenRange::Trim( const std::tstring& text )
{
	ASSERT( IsNormalized() );
	ASSERT( str::IsValidPos( m_start, text ) && str::IsValidPos( m_end, text ) );

	pred::IsSpace isSpace;

	while ( m_start != m_end && isSpace( text[ m_start ] ) )
		++m_start;

	while ( m_end != m_start && isSpace( text[ m_end - 1 ] ) )
		--m_end;
}

void TokenRange::ReplaceWithToken( std::tstring* pTargetText, const std::tstring& token )
{
	ASSERT_PTR( pTargetText );
	ASSERT( IsValid() && IsNormalized() );
	ASSERT( InStringBounds( pTargetText->c_str() ) );

	pTargetText->replace( m_start, getLength(), token );
	m_end = m_start + static_cast<int>( token.length() );
}

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
	ASSERT_PTR( pToken );

	if ( IsValid() && IsNormalized() )
	{
		int tokenLength = (int)str::GetLength( pToken );

		if ( getLength() == tokenLength )
			if ( caseType == str::Case )
				return pred::Equal == _tcsncmp( pText + m_start, pToken, tokenLength );
			else
				return pred::Equal == _tcsnicmp( pText + m_start, pToken, tokenLength );
	}

	return false;
}

// replace the pointed token with the new one in targetString, and updates m_end.

TokenRange& TokenRange::replaceWithToken( CString* pTargetString, const TCHAR* pToken )
{
	ASSERT_PTR( pTargetString );
	ASSERT( IsValid() && IsNormalized() );
	ASSERT( InStringBounds( *pTargetString ) && pToken != nullptr );

	pTargetString->Delete( m_start, getLength() );
	pTargetString->Insert( m_start, pToken );
	m_end = m_start + (int)str::GetLength( pToken );

	return *this;
}

TokenRange& TokenRange::smartReplaceWithToken( CString* pTargetString, const TCHAR* pToken )
{
	ASSERT_PTR( pTargetString );
	if ( !isTokenMatch( pTargetString->GetString(), pToken ) )
		replaceWithToken( pTargetString, pToken );

	return *this;
}
