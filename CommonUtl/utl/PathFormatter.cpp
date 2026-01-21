
#include "pch.h"
#include "PathFormatter.h"
#include "StringUtilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace hlp
{
	void CutLeadingDot( std::tstring& rExtension )
	{
		str::TrimLeft( rExtension, _T(".") );
	}

	void PrependLeadingDot( std::tstring& rExtension )
	{
		if ( !rExtension.empty() )
			rExtension = _T('.') + rExtension;
	}

	std::tstring FormatDigits( std::tstring::const_iterator& itFmt, std::tstring::const_iterator itFmtEnd, UINT num )
	{
		size_t digitCount = 0;
		for ( ; itFmt != itFmtEnd && _T('#') == *itFmt; ++itFmt )
			++digitCount;

		ASSERT( digitCount > 0 );

		std::tstring output = num::FormatNumber( num );
		if ( output.length() < digitCount )
			output.insert( output.begin(), digitCount - output.length(), _T('0') );
		return output;
	}

	std::tstring FormatPrintf( std::tstring::const_iterator& itFmt, std::tstring::const_iterator itFmtEnd, UINT num )
	{
		ASSERT( itFmt != itFmtEnd );

		std::tstring::const_iterator itStart = itFmt, itCursor = itFmt;

		if ( _T('%') == *itCursor )
		{
			// avoid treating "%%" as "%" (escape seq)
			static const std::tstring prinfNumericTypes = _T("diuoxX");

			++itCursor;
			while ( itCursor != itFmtEnd && str::CharTraits::IsDigit( *itCursor ) )
				++itCursor;

			if ( itCursor != itFmtEnd )
				if ( prinfNumericTypes.find( *itCursor ) != std::tstring::npos )
				{
					itFmt = ++itCursor;					// skip past the format spec
					std::tstring format( itStart, itFmt );
					return str::Format( format.c_str(), num );
				}
		}
		TRACE_FL( _T("> bad printf format syntax in \"%s\"\n"), &*itStart );
		return std::tstring();
	}

	bool ParseDigits( UINT& rNum, std::tistringstream& issSrc, std::tstring::const_iterator& itFmt, std::tstring::const_iterator itFmtEnd )
	{
		while ( itFmt != itFmtEnd && _T('#') == *itFmt )
			++itFmt;

		issSrc >> rNum;
		return !issSrc.fail();
	}

	bool ParseScanf( UINT& rNum, std::tistringstream& issSrc, std::tstring::const_iterator& itFmt, std::tstring::const_iterator itFmtEnd )
	{
		ASSERT( itFmt != itFmtEnd );

		std::tstring::const_iterator itStart = itFmt, itCursor = itFmt;

		if ( _T('%') == *itCursor )
		{
			// avoid treating "%%" as "%" (escape seq)
			static const std::tstring prinfNumericTypes = _T("diuoxX");

			++itCursor;
			while ( itCursor != itFmtEnd && str::CharTraits::IsDigit( *itCursor ) )
				++itCursor;

			if ( itCursor != itFmtEnd )
				if ( prinfNumericTypes.find( *itCursor ) != std::tstring::npos )
				{
					itFmt = ++itCursor;					// skip past the format spec
					issSrc >> rNum;
					return !issSrc.fail();
				}
		}
		TRACE_FL( _T("> bad scanf format syntax in \"%s\"\n"), &*itStart );
		return false;
	}
}


// CPathFormatter implementation

const std::tstring CPathFormatter::s_asIs = _T("*");

CPathFormatter::CPathFormatter( void )
	: m_format( s_asIs )
	, m_fnameFormat( m_format.Get() )
	, m_ignoreExtension( true )
	, m_isNumericFormat( false )
	, m_isWildcardFormat( true )
{
}

CPathFormatter::CPathFormatter( const std::tstring& format, bool ignoreExtension )
	: m_format( format )
	, m_ignoreExtension( ignoreExtension )
	, m_isNumericFormat( IsNumericFormat( m_format.Get() ) )
	, m_isWildcardFormat( path::ContainsWildcards( m_format.GetPtr() ) )		// was fs::CPathParts( format ).m_fname.c_str(), but it doesn't work for ".*" which should remove fname
{
	if ( m_ignoreExtension )
		m_fnameFormat = m_format.Get();
	else
	{
		m_format.SplitFilename( m_fnameFormat, m_extFormat );

		if ( IsValidFormat() )
		{
			if ( m_extFormat.empty() )
				m_extFormat = _T(".*");					// use ".*"
			else if ( _T(".") == m_extFormat )
				m_extFormat.clear();					// "." -> no extension

			hlp::CutLeadingDot( m_extFormat );
		}
	}
}

bool CPathFormatter::IsValidFormat( void ) const
{
	return
		m_isNumericFormat
		|| m_isWildcardFormat
		|| path::IsValidPath( m_format.Get() );		// even non-numeric or non-wildcard formats could yield unique results with files in distinct directories (no name collisions)
}

bool CPathFormatter::IsConsistent( void ) const
{
	if ( !IsValidFormat() )
		return false;

	if ( m_ignoreExtension )
		if ( !str::IsEmpty( m_format.GetExt() ) && path::ContainsWildcards( m_format.GetExt() ) )
			return false;

	return true;
}

CPathFormatter CPathFormatter::MakeConsistent( void ) const
{
	if ( IsValidFormat() && !IsConsistent() )
		return CPathFormatter( m_format.GetFname(), m_ignoreExtension );		// strip the extension in format

	return *this;
}

void CPathFormatter::SetMoveDestDirPath( const fs::CPath& moveDestDirPath )
{
	// note: moveDestDirPath could be empty for doc storage destination (root files)
	m_moveDestDirPath = moveDestDirPath;
}

bool CPathFormatter::IsNumericFormat( const std::tstring& format )
{
	if ( format.find( _T('#') ) != std::tstring::npos || format.find( _T('%') ) != std::tstring::npos )
	{
		static const TCHAR s_dummy[] = _T("dummy");
		std::tstring genPath1 = FormatPart( s_dummy, format, 1 );
		std::tstring genPath2 = FormatPart( s_dummy, format, 2 );

		return genPath1 != genPath2;
	}
	return false;
}

fs::CPath CPathFormatter::FormatPath( const fs::CPath& srcPath, UINT seqCount, UINT dupCount /*= 0*/ ) const
{
	const fs::CPath dirPath = !m_moveDestDirPath.IsEmpty() ? m_moveDestDirPath : srcPath.GetParentPath();

	std::tstring outFname, outExt;
	srcPath.SplitFilename( outFname, outExt );

	// format the filename: convert wildcards to filename source characters
	bool syntaxOk;
	outFname = FormatPart( outFname, m_fnameFormat, seqCount, &syntaxOk );
	if ( !syntaxOk )
		TRACE_FL( _T("> bad fname format syntax: '%s'\n"), m_fnameFormat.c_str() );

	if ( dupCount > 1 )
		outFname += str::Format( _T("_$(%d)"), dupCount );			// append dup count suffix

	if ( !m_ignoreExtension )
	{
		// format the extension: convert wildcards to extension source characters (skip the point from source, if any)
		hlp::CutLeadingDot( outExt );

		outExt = FormatPart( outExt, m_extFormat, 0, &syntaxOk );	// no counter for extension
		if ( !syntaxOk )
			TRACE_FL( _T("> bad extension format syntax: '%s'\n"), m_extFormat.c_str() );

		hlp::PrependLeadingDot( outExt );
	}

	return dirPath / ( outFname + outExt );
}

bool CPathFormatter::ParseSeqCount( UINT& rSeqCount, const fs::CPath& srcPath ) const
{
	return ParsePart( rSeqCount, m_fnameFormat, srcPath.GetFname() );
}

	// format uses tokens mixed with text:
	//	numeric:	"#", "##", "%d", "%02d", "%02X"
	//	wildcards:	"*.*", "?"

std::tstring CPathFormatter::FormatPart( const std::tstring& part, const std::tstring& format, UINT seqCount, bool* pSyntaxOk /*= nullptr*/ )
{
	std::vector<TCHAR> output;
	output.reserve( MAX_PATH );

	bool syntaxOk = true, doneSeq = false;
	std::tstring::const_iterator itPart = part.begin(), itPartEnd = part.end();

	for ( std::tstring::const_iterator itFmt = format.begin(); itFmt != format.end(); )
	{
		switch ( *itFmt )
		{
			case _T('#'):
				if ( !doneSeq )				// output sequence number only once
				{
					std::tstring numText = hlp::FormatDigits( itFmt, format.end(), seqCount );
					if ( !numText.empty() )
					{
						output.insert( output.end(), numText.begin(), numText.end() );
						doneSeq = true;
						continue;			// skip loop increment
					}
				}
				output.push_back( *itFmt );
				break;
			case _T('%'):
				if ( !doneSeq )
				{
					std::tstring numText = hlp::FormatPrintf( itFmt, format.end(), seqCount );
					if ( !numText.empty() )
					{
						output.insert( output.end(), numText.begin(), numText.end() );
						doneSeq = true;
						continue;			// skip loop increment
					}
				}
				output.push_back( *itFmt );
				break;
			case _T('*'):
				if ( itPart != itPartEnd )
				{	// copy the whole source string
					output.insert( output.end(), itPart, itPartEnd );
					itPart = itPartEnd;
				}
				else
					syntaxOk = false;
				break;
			case _T('?'):
				// copy the source character rather than the format character (if current source position is within the valid range)
				if ( itPart < itPartEnd )
					output.push_back( *itPart++ );
				else
					TRACE_FL( _T("> No source character for for wildcard '?' at position %d in source \"%s\"\n"),
							  std::distance( part.begin(), itPart ), part.c_str() );
				break;
			default:
				output.push_back( *itFmt );
				break;
		}
		++itFmt;
	}

	if ( output.empty() && !format.empty() )
		syntaxOk = false;					// not empty (exclude EOS)

	if ( pSyntaxOk != nullptr )
		*pSyntaxOk = syntaxOk;

	output.push_back( _T('\0') );			// end the string

	return &output.front();
}

bool CPathFormatter::ParsePart( UINT& rSeqCount, const std::tstring& format, const std::tstring& part )
{
	rSeqCount = 0;
	std::tistringstream issSrc( part );
	issSrc.unsetf( std::ios::skipws );			// count whitespace as regular character

	bool doneSeq = false;
	TCHAR chSrc;

	for ( std::tstring::const_iterator itFmt = format.begin(), itFmtEnd = format.end(); itFmt != format.end() && !issSrc.eof(); )
	{
		switch ( *itFmt )
		{
			case _T('#'):
				if ( !doneSeq )				// output sequence number only once
					if ( hlp::ParseDigits( rSeqCount, issSrc, itFmt, itFmtEnd ) )
					{
						doneSeq = true;
						continue;			// skip loop increment
					}
					else
						return false;

				issSrc >> chSrc;
				break;
			case _T('%'):
				if ( !doneSeq )
					if ( hlp::ParseScanf( rSeqCount, issSrc, itFmt, itFmtEnd ) )
					{
						doneSeq = true;
						continue;			// skip loop increment
					}

				issSrc >> chSrc;
				break;
			case _T('*'):
				if ( !SkipStar( issSrc, itFmt, itFmtEnd ) )
					return false;					// mismatch
				continue;
			case _T('?'):
				issSrc >> chSrc;
				break;
			default:
				issSrc >> chSrc;
				if ( !pred::TCharEqualEquivalentPath()( *itFmt, chSrc ) )
					return false;					// mismatch
				break;
		}
		++itFmt;
	}

	return doneSeq;
}

bool CPathFormatter::SkipStar( std::tistringstream& issSrc, std::tstring::const_iterator& itFmt, std::tstring::const_iterator itFmtEnd )
{
	ASSERT( _T('*') == *itFmt );
	++itFmt;
	if ( itFmt == itFmtEnd )						// last '*' in format
		issSrc.seekg( 0, std::ios_base::end );		// eat remaining source
	else
	{
		for ( ; itFmt != itFmtEnd && !issSrc.eof(); )
		{
			TCHAR chSrc;
			issSrc >> chSrc;

			switch ( *itFmt )
			{
				case _T('#'):
				case _T('%'):
					if ( str::CharTraits::IsDigit( chSrc ) )
					{
						issSrc.unget();
						return true;
					}
					else
						return false;				// mismatch
					break;
				case _T('?'):
					++itFmt;
					break;
				default:
					if ( pred::TCharEqualEquivalentPath()( *itFmt, chSrc ) )
					{
						issSrc.unget();
						return true;
					}
			}
		}
	}

	return true;
}
