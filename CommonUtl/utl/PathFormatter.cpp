
#include "stdafx.h"
#include "PathFormatter.h"
#include "ContainerUtilities.h"
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
		TRACE( _T("> bad printf format syntax in \"%s\"\n"), &*itStart );
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
		TRACE( _T("> bad scanf format syntax in \"%s\"\n"), &*itStart );
		return false;
	}
}


// CPathFormatter implementation

CPathFormatter::CPathFormatter( const std::tstring& format )
	: m_isNumericFormat( IsNumericFormat( format ) )
	, m_isWildcardFormat( path::ContainsWildcards( format.c_str() ) )		// was fs::CPathParts( format ).m_fname.c_str(), but it doesn't work for ".*" which should remove fname
	, m_formatParts( format )
{
	if ( IsValidFormat() )
	{
		if ( m_formatParts.m_ext.empty() )
			m_formatParts.m_ext = _T(".*");					// no extension -> use ".*"
		else if ( _T(".") == m_formatParts.m_ext )
			m_formatParts.m_ext.clear();					// "." -> no extension

		hlp::CutLeadingDot( m_formatParts.m_ext );
	}
}

void CPathFormatter::SetMoveDestDirPath( const std::tstring& moveDestDirPath )
{
	// note: moveDestDirPath could be empty for doc storage destination (root files)
	m_pMoveDestDirPath.reset( new fs::CPathParts( moveDestDirPath ) );
}

bool CPathFormatter::IsNumericFormat( const std::tstring& format )
{
	if ( format.find( _T('#') ) != std::tstring::npos || format.find( _T('%') ) != std::tstring::npos )
	{
		static const TCHAR dummy[] = _T("dummy");
		std::tstring genPath1 = FormatPart( format, dummy, 1 );
		std::tstring genPath2 = FormatPart( format, dummy, 2 );
		return genPath1 != genPath2;
	}
	return false;
}

fs::CPath CPathFormatter::FormatPath( const std::tstring& srcPath, UINT seqCount, UINT dupCount /*= 0*/ ) const
{
	fs::CPathParts output( srcPath );

	if ( m_pMoveDestDirPath.get() != NULL )		// use a single destination directory to move files?
	{
		output.m_drive = m_pMoveDestDirPath->m_drive;
		output.m_dir = m_pMoveDestDirPath->m_dir;
	}

	// format the filename: convert wildcards to filename source characters
	bool syntaxOk;
	output.m_fname = FormatPart( m_formatParts.m_fname, output.m_fname, seqCount, &syntaxOk );
	if ( !syntaxOk )
		TRACE( _T("> bad fname format syntax: '%s'\n"), m_formatParts.m_fname.c_str() );

	if ( dupCount > 1 )
		output.m_fname += str::Format( _T("_$(%d)"), dupCount );						// append dup count suffix

	// format the extension: convert wildcards to extension source characters (skip the point from source, if any)
	hlp::CutLeadingDot( output.m_ext );

	output.m_ext = FormatPart( m_formatParts.m_ext, output.m_ext, 0, &syntaxOk );		// no counter for extension
	if ( !syntaxOk )
		TRACE( _T("> bad extension format syntax: '%s'\n"), m_formatParts.m_ext.c_str() );

	hlp::PrependLeadingDot( output.m_ext );

	return output.MakePath();
}

bool CPathFormatter::ParseSeqCount( UINT& rSeqCount, const std::tstring& srcPath ) const
{
	fs::CPathParts srcParts( srcPath );
	return ParsePart( rSeqCount, m_formatParts.m_fname, srcParts.m_fname );
}

	// format uses tokens mixed with text:
	//	numeric:	"#", "##", "%d", "%02d", "%02X"
	//	wildcards:	"*.*", "?"

std::tstring CPathFormatter::FormatPart( const std::tstring& format, const std::tstring& src, UINT seqCount, bool* pSyntaxOk /*= NULL*/ )
{
	std::vector< TCHAR > output;
	output.reserve( _MAX_PATH );

	bool syntaxOk = true, doneSeq = false;
	std::tstring::const_iterator itSrc = src.begin(), itSrcEnd = src.end();

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
				if ( itSrc != itSrcEnd )
				{	// copy the whole source string
					output.insert( output.end(), itSrc, itSrcEnd );
					itSrc = itSrcEnd;
				}
				else
					syntaxOk = false;
				break;
			case _T('?'):
				// copy the source character rather than the format character (if current source position is within the valid range)
				if ( itSrc < itSrcEnd )
					output.push_back( *itSrc++ );
				else
					TRACE( _T("> No source character for for wildcard '?' at position %d in source \"%s\"\n"),
						   std::distance( src.begin(), itSrc ), src.c_str() );
				break;
			default:
				output.push_back( *itFmt );
				break;
		}
		++itFmt;
	}

	if ( output.empty() && !format.empty() )
		syntaxOk = false;					// not empty (exclude EOS)

	if ( pSyntaxOk != NULL )
		*pSyntaxOk = syntaxOk;

	output.push_back( _T('\0') );			// end the string

	return &output.front();
}

bool CPathFormatter::ParsePart( UINT& rSeqCount, const std::tstring& format, const std::tstring& src )
{
	rSeqCount = 0;
	std::tistringstream issSrc( src );
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
				if ( !pred::EquivalentPathChar()( *itFmt, chSrc ) )
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
					if ( pred::EquivalentPathChar()( *itFmt, chSrc ) )
					{
						issSrc.unget();
						return true;
					}
			}
		}
	}

	return true;
}
