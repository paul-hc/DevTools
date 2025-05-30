
#include "pch.h"
#include "StringUtilitiesEx.h"
#include "utl/RuntimeException.h"
#include <atlconv.h>
#include <algorithm>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


#ifdef _UNICODE

CString readLineFromStream( std::istream& rIs, int maxLineSize /*= 2000*/ )
{
	std::auto_ptr<char> buffer( new char[ maxLineSize ] );

	rIs.getline( buffer.get(), maxLineSize );

	CString lineString;

	AtlA2WHelper( lineString.GetBuffer( maxLineSize ), buffer.get(), maxLineSize );
	lineString.ReleaseBuffer();

	return lineString;
}

CString readLineFromStream( std::wistream& rIs, int maxLineSize /*= 2000*/ )
{
	ASSERT( rIs.good() && maxLineSize > 0 );

	CString lineString;

	rIs.getline( lineString.GetBuffer( maxLineSize ), maxLineSize );
	lineString.ReleaseBuffer();

	return lineString;
}


#else // _UNICODE

/**
	ANSI versions
*/

std::ostream& operator<<( std::ostream& rOs, const CString& rString )
{
	return rOs << (const TCHAR*)rString;
}

std::wostream& operator<<( std::wostream& rOs, const CString& rString )
{
	const size_t bufferSize = rString.GetLength() + 1;
	std::auto_ptr<wchar_t> wideBuffer( new wchar_t[ bufferSize ] );

	AtlA2WHelper( wideBuffer.get(), rString, bufferSize );
	return rOs << wideBuffer.get();
}

CString readLineFromStream( std::istream& rIs, int maxLineSize /*= 2000*/ )
{
	ASSERT( rIs.good() && maxLineSize > 0 );

	CString lineString;

	rIs.getline( lineString.GetBuffer( maxLineSize ), maxLineSize );
	lineString.ReleaseBuffer();

	return lineString;
}

CString readLineFromStream( std::wistream& rIs, int maxLineSize /*= 2000*/ )
{
	std::auto_ptr<wchar_t> buffer( new wchar_t[ maxLineSize ] );

	rIs.getline( buffer.get(), maxLineSize );

	CString lineString;

	AtlW2AHelper( lineString.GetBuffer( maxLineSize ), buffer.get(), maxLineSize );
	lineString.ReleaseBuffer();

	return lineString;
}


#endif // _UNICODE


namespace str
{
	CString formatString( const TCHAR* format, ... )
	{
		va_list argList;

		va_start( argList, format );

		CString message;

		message.FormatV( format, argList );
		va_end( argList );

		return message;
	}

	CString formatString( UINT formatResId, ... )
	{
		va_list argList;

		va_start( argList, formatResId );

		CString format( MAKEINTRESOURCE( formatResId ) );
		CString message;

		message.FormatV( format, argList );
		va_end( argList );

		return message;
	}

	// doesn't clear rOutDestTokens, it appends tokens to it
	size_t split( std::vector<CString>& rOutDestTokens, const TCHAR* flatString, const TCHAR* separator )
	{
		ASSERT( separator != nullptr && *separator != '\0' );

		int separatorLength = str::Length( separator );
		int startPos = 0;

		if ( flatString != nullptr && flatString[ 0 ] != '\0' )
			for ( ; ; )
			{
				const TCHAR* foundSep = _tcsstr( flatString + startPos, separator );
				TokenRange tokenRange( startPos, foundSep != nullptr ? int( foundSep - flatString ) : -1 );

				if ( foundSep != nullptr )
				{
					rOutDestTokens.push_back( tokenRange.getString( flatString ) );
					startPos = tokenRange.m_end + separatorLength;
				}
				else
				{
					tokenRange.normalize( flatString );
					rOutDestTokens.push_back( tokenRange.getString( flatString ) );
					break;
				}
			}

		return rOutDestTokens.size();
	}

	// doesn't clear rOutDestTokens, it appends tokens to it
	size_t tokenize( std::vector<CString>& rOutDestTokens, const TCHAR* flatString, const TCHAR* separators )
	{
		CString flatCopy = flatString;

		if ( !flatCopy.IsEmpty() )
			for ( TCHAR* pToken = _tcstok( (TCHAR*)(const TCHAR*)flatCopy, separators ); pToken != nullptr; pToken = _tcstok( nullptr, separators ) )
			{
				CString item( pToken );

				item.TrimLeft();
				item.TrimRight();
				if ( !item.IsEmpty() )
					rOutDestTokens.push_back( item );
			}

		return rOutDestTokens.size();
	}

	CString unsplit( std::vector<CString>::const_iterator startToken, std::vector<CString>::const_iterator endToken,
					 const TCHAR* separator )
	{
		int separatorLength = str::Length( separator );
		int requiredBufferLength = 1; // include zero-terminator

		std::vector<CString>::const_iterator tokenIter;

		for ( tokenIter = startToken; tokenIter != endToken; ++tokenIter )
			requiredBufferLength += ( *tokenIter ).GetLength();

		int tokenCount = static_cast<int>( endToken - startToken );

		if ( tokenCount > 1 )
			requiredBufferLength += ( tokenCount - 1 ) * separatorLength;

		CString outFlatString;

		outFlatString.GetBuffer( requiredBufferLength );
		outFlatString.ReleaseBuffer( requiredBufferLength );
		outFlatString.Empty();

		for ( tokenIter = startToken; tokenIter != endToken; ++tokenIter )
		{
			if ( tokenIter != startToken )
				outFlatString += separator;
			outFlatString += *tokenIter;
		}

		return outFlatString;
	}

	bool parseInteger( int& rOutNumber, const TCHAR* pString )
	{
		ASSERT( pString != nullptr );

		const TCHAR* ptr = pString;

		while ( *ptr != '\0' && _istspace( *ptr ) )
			++ptr;

		const TCHAR* format = _T("%d");

		if ( 0 == _tcsncmp( ptr, _T("\"\\"), 2 ) )
		{	// ex: "\"\\23\"", "\"xFD\"", "\"\\023\""
			ptr += 2;
			switch ( _totlower( *ptr ) )
			{
				case _T('0'): format = _T("%o"); ++ptr; break;
				case _T('x'): format = _T("%x"); ++ptr; break;
			}
		}
		else if ( *ptr == _T('0') && *( ptr + 1 ) != '\0' )
			if ( _totlower( *++ptr ) == _T('x') )
			{
				format = _T("%x"); // hex
				++ptr;
			}
			else
				format = _T("%o"); // octal

		return 1 == _stscanf( ptr, format, &rOutNumber );
	}

	int parseInteger( const TCHAR* pString ) throws_( CRuntimeException )
	{
		int number = 0;

		if ( !parseInteger( number, pString ) )
			throw CRuntimeException( str::Format( _T("invalid integer number '%s'"), pString ), UTL_FILE_LINE );

		return number;
	}

	NumType parseUnsignedInteger( unsigned int& rOutNumber, const TCHAR* pString )
	{
		ASSERT_PTR( pString );

		const TCHAR* ptr = pString;

		while ( *ptr != '\0' && ::_istspace( *ptr ) )
			++ptr;

		NumType numType = DecimalNum;

		if ( 0 == ::_tcsncmp( ptr, _T("\"\\"), 2 ) )
		{	// ex: "\"\\23\"", "\"xFD\"", "\"\\023\""
			ptr += 2;

			switch ( ::_totlower( *ptr ) )
			{
				case _T('x'): numType = HexNum; ++ptr; break;
				case _T('0'): numType = OctalNum; ++ptr; break;
			}
		}
		else if ( _T('0') == *ptr && *( ptr + 1 ) != '\0' )
			if ( _T('x') == ::_totlower( *++ptr ) )
			{
				numType = HexNum;
				++ptr;
			}
			else
				numType = DecimalNum;		// assume zero-padded decimal

		static const TCHAR* numFormats[] = { _T("%u"), _T("%x"), _T("%o") };

		return 1 == ::_stscanf( ptr, numFormats[ numType ], &rOutNumber )
			? numType
			: NoNumber;
	}

	unsigned int parseUnsignedInteger( const TCHAR* pString ) throws_( CRuntimeException )
	{
		unsigned int number = 0;
		if ( NoNumber == parseUnsignedInteger( number, pString ) )
			throw CRuntimeException( str::Format( _T("invalid unsigned int number '%s'"), pString ), UTL_FILE_LINE );

		return number;
	}

	double parseDouble( const TCHAR* pString )
	{
		return atof( str::ToAnsi( pString ).c_str() );
	}


	int findCharPos( const TCHAR* pString, TCHAR chr, int startPos /*= 0*/, str::CaseType caseType /*= str::Case*/ )
	{
		ASSERT( startPos >= 0 && startPos <= str::Length( pString ) );

		typedef const TCHAR* TConstIterator;

		TConstIterator itEnd = end( pString ), itFound;

		if ( caseType == str::Case )
			itFound = std::find_if( begin( pString ) + startPos, itEnd, CharMatchCase( chr ) );
		else
			itFound = std::find_if( begin( pString ) + startPos, itEnd, CharMatchNoCase( chr ) );

		return itFound != itEnd ? int( itFound - begin( pString ) ) : -1;
	}

	TokenRange findStringPos( const TCHAR* pString, const TCHAR* subString, int startPos /*= 0*/,
							  str::CaseType caseType /*= str::Case*/ )
	{
		ASSERT( startPos >= 0 && startPos <= str::Length( pString ) );

		TConstIterator itFound;

		if ( caseType == str::Case )
			itFound = std::search( begin( pString ) + startPos, end( pString ), begin( subString ), end( subString ) );
		else
			itFound = std::search( begin( pString ) + startPos, end( pString ), begin( subString ), end( subString ),
								   CharEqualNoCase() );

		TokenRange foundRange( -1 );

		if ( itFound != end( pString ) )
		{
			foundRange.setWithLength( int( itFound - begin( pString ) ), str::Length( subString ) );
	//		TRACE( _T("FOUND in '%s' sub '%s' at SP='%s' EP='%s'\n"), pString, subString, pString + foundRange.m_start, pString + foundRange.m_end );
		}

		return foundRange;
	}

	TokenRange reverseFindStringPos( const TCHAR* pString, const TCHAR* subString, int startPos /*= -1*/,
									 str::CaseType caseType /*= str::Case*/ )
	{
		if ( startPos == -1 )
			startPos = str::Length( pString );

		ASSERT( startPos >= 0 && startPos <= str::Length( pString ) );

		str::const_reverse_iterator itFound;

		if ( caseType == str::Case )
			itFound = std::search( rbegin( pString, startPos ), rend( pString ), rbegin( subString ), rend( subString ) );
		else
			itFound = std::search( rbegin( pString, startPos ), rend( pString ), rbegin( subString ), rend( subString ),
								   CharEqualNoCase() );

		TokenRange foundRange( -1, -1 );

		if ( itFound != rend( pString ) )
		{
			foundRange.setEmpty( int( rend( pString ) - itFound ) );
			foundRange.m_start -= str::Length( subString );

	//		TRACE( _T("REVERSE FOUND in '%s' sub '%s' at SP='%s' EP='%s'\n"), pString, subString, pString + foundRange.m_start, pString + foundRange.m_end );
		}

		return foundRange;
	}

	int findOneOfPos( const TCHAR* pString, const TCHAR* charSet, int startPos /*= 0*/,
					  str::CaseType caseType /*= str::Case*/ )
	{
		ASSERT( startPos >= 0 && startPos <= str::Length( pString ) );

		typedef const TCHAR* TConstIterator;
		TConstIterator itFound;

		if ( caseType == str::Case )
			itFound = std::find_first_of( begin( pString ) + startPos, end( pString ), begin( charSet ), end( charSet ) );
		else
			itFound = std::find_first_of( begin( pString ) + startPos, end( pString ), begin( charSet ), end( charSet ),
										  CharEqualNoCase() );

		int foundPos = -1;

		if ( itFound != end( pString ) )
		{
			foundPos = int( itFound - begin( pString ) );
	//		TRACE( _T("FOUND in '%s' ANY_OF_SET '%s' at '%s'\n"), pString, charSet, pString + foundPos );
		}

		return foundPos;
	}

	int reverseFindOneOfPos( const TCHAR* pString, const TCHAR* charSet, int startPos /*= -1*/,
							 str::CaseType caseType /*= str::Case*/ )
	{
		if ( startPos == -1 )
			startPos = str::Length( pString );

		ASSERT( startPos >= 0 && startPos <= str::Length( pString ) );

		str::const_reverse_iterator itFound;

		if ( caseType == str::Case )
			itFound = std::find_first_of( rbegin( pString, startPos ), rend( pString ), begin( charSet ), end( charSet ) );
		else
			itFound = std::find_first_of( rbegin( pString, startPos ), rend( pString ), begin( charSet ), end( charSet ),
										  CharEqualNoCase() );

		int foundPos = -1;

		if ( itFound != rend( pString ) )
		{
			foundPos = int( rend( pString ) - itFound );

	//		TRACE( _T("REVERSE FOUND in '%s' ANY_OF_SET '%s' at '%s'\n"), pString, charSet, pString + foundPos );
		}

		return foundPos;
	}

	/**
		Returns true if the specified token matches at startPos
	*/
	bool isTokenMatch( const TCHAR* pString, const TCHAR* token, int startPos /*= 0*/, str::CaseType caseType /*= str::Case*/ )
	{
		ASSERT( token != nullptr );
		ASSERT( pString != nullptr && startPos >= 0 && startPos <= str::Length( pString ) );

		int tokenLength = str::Length( token );

		return caseType == str::Case ? ( _tcsncmp( pString + startPos, token, tokenLength ) == 0 )
										 : ( _tcsnicmp( pString + startPos, token, tokenLength ) == 0 );
	}


	int stringReplace( CString& rString, const TCHAR* match, const TCHAR* replacement,
					   str::CaseType caseType /*= str::Case*/ )
	{
		int matchLen = str::Length( match ), replacementLen = str::Length( replacement );
		int replacementCount = 0, foundPos = 0;

		if ( !rString.IsEmpty() && matchLen > 0 )
			do
			{
				foundPos = findStringPos( (LPCTSTR)rString, match, foundPos, caseType ).m_start;
				if ( foundPos != -1 )
				{
					++replacementCount;
					rString.Delete( foundPos, matchLen );
					rString.Insert( foundPos, replacement );
					foundPos += replacementLen;
				}
			} while ( foundPos != -1 );

		return replacementCount;
	}

} // namespace str
