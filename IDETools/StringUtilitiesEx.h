#ifndef StringUtilitiesEx_h
#define StringUtilitiesEx_h
#pragma once


#include <functional>
#include <algorithm>
#include <map>
#include <iosfwd>
#include "TokenRange.h"


CString readLineFromStream( std::istream& rIs, int maxLineSize = 2000 );
CString readLineFromStream( std::wistream& rIs, int maxLineSize = 2000 );


namespace str
{
	CString formatString( const TCHAR* format, ... );
	CString formatString( UINT formatResId, ... );

	inline int Length( const TCHAR* pString ) { return static_cast<int>( GetLength( pString ) ); }	// return int for convenience


	// tokenize/untokenize

	size_t split( std::vector< CString >& rOutDestTokens, const TCHAR* flatString, const TCHAR* separator );
	size_t tokenize( std::vector< CString >& rOutDestTokens, const TCHAR* flatString, const TCHAR* separators );

	CString unsplit( std::vector< CString >::const_iterator startToken, std::vector< CString >::const_iterator endToken, const TCHAR* separator );
	CString unsplit( const std::vector< CString >& rSrcTokens, const TCHAR* separator );

	template< typename Type, typename EqualPred >
	void removeDuplicates( std::vector< Type >& rTarget, EqualPred isEqual );


	// numeric conversions

	bool parseInteger( int& rOutNumber, const TCHAR* pString );
	int parseInteger( const TCHAR* pString ) throws_( CRuntimeException );

	enum NumType { DecimalNum, HexNum, OctalNum, NoNumber };

	NumType parseUnsignedInteger( unsigned int& rOutNumber, const TCHAR* pString );
	unsigned int parseUnsignedInteger( const TCHAR* pString ) throws_( CRuntimeException );

	double parseDouble( const TCHAR* pString );

	bool isNumberChar( TCHAR chr );


	// whitespace processing

	template< typename IntegralType >
	void skipWhiteSpace( IntegralType& rPos, const TCHAR* str )
	{
		ASSERT( str != NULL );
		ASSERT( rPos >= (IntegralType)0 && rPos <= (IntegralType)GetLength( str ) );

		while ( str[ rPos ] != _T('\0') && _istspace( str[ rPos ] ) )
			++rPos;
	}


	template< typename IntegralType >
	void skipNonWhiteSpace( IntegralType& rPos, const TCHAR* str )
	{
		ASSERT( str != NULL );
		ASSERT( rPos >= (IntegralType)0 && rPos <= (IntegralType)GetLength( str ) );

		while ( str[ rPos ] != _T('\0') && !_istspace( str[ rPos ] ) )
			++rPos;
	}


	template< typename IntegralType >
	void skipDigit( IntegralType& rPos, const TCHAR* str )
	{
		ASSERT( str != NULL );
		ASSERT( rPos >= (IntegralType)0 && rPos <= (IntegralType)GetLength( str ) );
		ASSERT( isNumberChar( str[ rPos ] ) );

		bool isHexNumber = 0 == _tcsnicmp( str + rPos, _T("0x"), 2 );

		if ( isHexNumber )
			rPos += 2;

		while ( isHexNumber ? _istxdigit( str[ rPos ] ) : isNumberChar( str[ rPos ] ) )
			++rPos;
	}


	template< typename IntegralType >
	void skipCharSet( IntegralType& rPos, const TCHAR* str, const TCHAR* characterSet )
	{
		ASSERT( str != NULL );
		ASSERT( rPos >= (IntegralType)0 && rPos <= (IntegralType)GetLength( str ) );

		while ( str[ rPos ] != _T('\0') && str::isCharOneOf( str[ rPos ], characterSet ) )
			++rPos;
	}


	template< typename IntegralType >
	void skipNotCharSet( IntegralType& rPos, const TCHAR* str, const TCHAR* characterSet )
	{
		ASSERT( str != NULL );
		ASSERT( rPos >= (IntegralType)0 && rPos <= (IntegralType)GetLength( str ) );

		while ( str[ rPos ] != _T('\0') && !str::isCharOneOf( str[ rPos ], characterSet ) )
			++rPos;
	}


	template< typename IntegralType >
	bool skipToken( IntegralType& rPos, const TCHAR* str, const TCHAR* token, str::CaseType caseType = str::Case )
	{
		ASSERT( str != NULL && token != NULL );

		if ( !str::isTokenMatch( str, token, rPos, caseType ) )
			return false;

		rPos += str::Length( token );
		str::skipWhiteSpace( rPos, str );
		return true;
	}


	// string search

	int findCharPos( const TCHAR* pString, TCHAR chr, int startPos = 0, str::CaseType caseType = str::Case );

	TokenRange findStringPos( const TCHAR* pString, const TCHAR* subString, int startPos = 0,
							  str::CaseType caseType = str::Case );
	TokenRange reverseFindStringPos( const TCHAR* pString, const TCHAR* subString, int startPos = -1,
									 str::CaseType caseType = str::Case );
	int findOneOfPos( const TCHAR* pString, const TCHAR* charSet, int startPos = 0, str::CaseType caseType = str::Case );
	int reverseFindOneOfPos( const TCHAR* pString, const TCHAR* charSet, int startPos = -1, str::CaseType caseType = str::Case );

	TCHAR* findString( const TCHAR* pString, const TCHAR* subString, str::CaseType caseType = str::Case );
	TCHAR* reverseFindString( const TCHAR* pString, const TCHAR* subString, str::CaseType caseType = str::Case );


	// string matching

	bool isCharOneOf( TCHAR chr, const TCHAR* characterSet, str::CaseType caseType = str::Case );
	bool isTokenMatch( const TCHAR* pString, const TCHAR* token, int startPos = 0, str::CaseType caseType = str::Case );


	// string pointer access & safe position

	const TCHAR* ptrAt( const TCHAR* pString, int pos );
	TCHAR charAt( const TCHAR* pString, int pos );
	int safePos( int pos, const TCHAR* pString );
	int safePos( int pos, int endPos );

	int stringReplace( CString& rString, const TCHAR* match, const TCHAR* replacement, str::CaseType caseType = str::Case );


	// string to STL iterators conversions

	inline const_iterator end( const TCHAR* pString, int endPos )
	{
		if ( -1 == endPos )
			endPos = Length( pString );

		ASSERT( endPos >= 0 && endPos <= Length( pString ) );
		return pString + endPos;
	}

    typedef std::reverse_iterator< const_iterator > const_reverse_iterator;

	inline const_reverse_iterator rbegin( const TCHAR* pString, int startPos = -1 )
	{
		return const_reverse_iterator( end( pString, startPos ) );
	}

	inline const_reverse_iterator rend( const TCHAR* pString )
	{
		return const_reverse_iterator( begin( pString ) );
	}


	// binary predicates

	struct StringLess
	{
		bool operator()( const TCHAR* left, const TCHAR* right ) const
		{
			return _tcsicoll( left, right ) < 0;
		}
	};

	struct StringGreater
	{
		bool operator()( const TCHAR* left, const TCHAR* right ) const
		{
			return _tcsicoll( left, right ) > 0;
		}
	};

	struct CharEqualCase
	{
		bool operator()( TCHAR chLeft, TCHAR chRight ) const
		{
			return chLeft == chRight;
		}
	};

	struct CharEqualNoCase
	{
		bool operator()( TCHAR chLeft, TCHAR chRight ) const
		{
			return _totlower( chLeft ) == _totlower( chRight );
		}
	};

	struct CharEqual
	{
		CharEqual( str::CaseType _caseType ) : caseType( _caseType ) {}

		bool operator()( TCHAR chLeft, TCHAR chRight ) const
		{
			return caseType == str::Case ? ( chLeft == chRight )
											 : ( _totlower( chLeft ) == _totlower( chRight ) );
		}
	public:
		str::CaseType caseType;
	};


	// unary predicates

	struct CharMatchCase
	{
		CharMatchCase( TCHAR _match ) : match( _match ) {}

		bool operator()( TCHAR chr ) const
		{
			return chr == match;
		}
	public:
		TCHAR match;
	};

	struct CharMatchNoCase
	{
		CharMatchNoCase( TCHAR _match ) : lowerMatch( _totlower( _match ) ) {}

		bool operator()( TCHAR chr ) const
		{
			return _totlower( chr ) == lowerMatch;
		}
	public:
		TCHAR lowerMatch;
	};


	// inline code

	inline CString unsplit( const std::vector< CString >& rSrcTokens, const TCHAR* separator )
	{
		return unsplit( rSrcTokens.begin(), rSrcTokens.end(), separator );
	}


	inline bool isNumberChar( TCHAR chr )
	{
		return _istdigit( chr ) || chr == _T('-') || chr == _T('+');
	}

	template< typename Type, typename EqualPred >
	inline void removeDuplicates( std::vector< Type >& rTarget, EqualPred isEqualPred )
	{
		for ( typename std::vector< Type >::iterator it = rTarget.begin(); it != rTarget.end(); ++it )
		{
			for ( typename std::vector< Type >::iterator itTrailing = it + 1; itTrailing != rTarget.end(); )
				if ( isEqualPred( *itTrailing, *it ) )
					itTrailing = rTarget.erase( itTrailing );
				else
					++itTrailing;
		}
	}

	inline bool isCharOneOf( TCHAR chr, const TCHAR* characterSet, str::CaseType caseType /*= str::Case*/ )
	{
		ASSERT( characterSet != NULL );

		const_iterator itEnd = end( characterSet );

		if ( caseType == str::Case )
			return std::find( begin( characterSet ), itEnd, chr ) != itEnd;
		else
			return std::find_if( begin( characterSet ), itEnd, CharMatchNoCase( chr ) ) != itEnd;
	}

	inline TCHAR* findString( const TCHAR* pString, const TCHAR* subString, str::CaseType caseType /*= str::Case*/ )
	{
		size_t foundPos = findStringPos( pString, subString, 0, caseType ).m_start;

		return foundPos != -1 ? const_cast<TCHAR*>( pString + foundPos ) : NULL;
	}

	inline TCHAR* reverseFindString( const TCHAR* pString, const TCHAR* subString, str::CaseType caseType /*= str::Case*/ )
	{
		size_t foundPos = reverseFindStringPos( pString, subString, -1, caseType ).m_start;

		return foundPos != -1 ? const_cast<TCHAR*>( pString + foundPos ) : NULL;
	}

	inline const TCHAR* ptrAt( const TCHAR* pString, int pos )
	{
		ASSERT( pString != NULL && pos >= 0 && pos <= Length( pString ) );

		return pString + pos;
	}

	inline TCHAR charAt( const TCHAR* pString, int pos )
	{
		ASSERT( pString != NULL && pos >= 0 && pos <= Length( pString ) );

		return pString[ pos ];
	}

	inline int safePos( int pos, const TCHAR* pString )
	{
		ASSERT( pString != NULL && ( pos == -1 || pos <= Length( pString ) ) );

		if ( pos == -1 )
			pos = Length( pString );

		return pos;
	}

	inline int safePos( int pos, int endPos )
	{
		ASSERT( pos == -1 || pos <= endPos );

		if ( pos == -1 )
			pos = endPos;

		return pos;
	}

} // namespace str


#endif // StringUtilitiesEx_h
