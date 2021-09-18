#ifndef TextFileUtils_hxx
#define TextFileUtils_hxx

#include "stdafx.h"
#include "TextFileUtils.h"
#include "Path.h"
#include <fstream>


// line streaming template code

namespace utl
{
	// write container of lines to UTF8 text file

	template< typename LinesT >
	void WriteLinesToFile( const fs::CPath& targetFilePath, const LinesT& srcLines ) throws_( CRuntimeException )
	{
		std::ofstream output( targetFilePath.GetUtf8().c_str() );
		if ( !output.is_open() )
			throw CRuntimeException( str::Format( _T("Cannot write to text file: %s."), targetFilePath.GetPtr() ) );

		WriteLinesToStream( output, srcLines );		// save all lines
		output.close();
	}

	template< typename LinesT >
	void WriteLinesToStream( std::ostream& os, const LinesT& srcLines )
	{
		size_t linePos = 0;
		for ( typename LinesT::const_iterator itLine = srcLines.begin(); itLine != srcLines.end(); ++itLine )
			WriteTextLine( os, *itLine, &linePos );
	}


	// string read/write in UTF8 text file

	template< typename StringT >
	void ReadStringFromFile( StringT& rText, const fs::CPath& srcFilePath ) throws_( CRuntimeException )
	{
		std::ifstream input( srcFilePath.GetUtf8().c_str() );
		if ( !input.is_open() )
			throw CRuntimeException( str::Format( _T("Cannot read from text file: %s"), srcFilePath.GetPtr() ) );

		ReadText( input, rText );		// verbatim text string in text mode: with line-end translation
		input.close();
	}

	template< typename StringT >
	void WriteStringToFile( const fs::CPath& targetFilePath, const StringT& text ) throws_( CRuntimeException )
	{
		std::ofstream output( targetFilePath.GetUtf8().c_str() );
		if ( !output.is_open() )
			throw CRuntimeException( str::Format( _T("Cannot write to text file: %s."), targetFilePath.GetPtr() ) );

		WriteText( output, text );		// verbatim text string in text mode: with line-end translation
		output.close();
	}
}


// CTextFileParser template code

template< typename StringT >
void CTextFileParser< StringT >::ParseFile( const fs::CPath& srcFilePath ) throws_( CRuntimeException )
{
	// assume the text file is encoded in UTF8 (no BOM)
	std::ifstream input( srcFilePath.GetUtf8().c_str() );
	if ( !input.is_open() )
		throw CRuntimeException( str::Format( _T("Cannot read from text file: %s"), srcFilePath.GetPtr() ) );

	ParseStream( input );
	input.close();
}

template< typename StringT >
void CTextFileParser< StringT >::ParseStream( std::istream& is )
{
	Clear();

	if ( m_pLineParserCallback != NULL )
		m_pLineParserCallback->OnBeginParsing();

	for ( unsigned int lineNo = 1; !is.eof() && lineNo <= m_maxLineCount; ++lineNo )
	{
		StringT line;

		utl::ReadTextLine( is, line );

		if ( NULL == m_pLineParserCallback )
			m_parsedLines.push_back( line );
		else if ( !m_pLineParserCallback->OnParseLine( line, lineNo ) )
			break;
	}

	if ( m_pLineParserCallback != NULL )
		m_pLineParserCallback->OnEndParsing();
}


#endif // TextFileUtils_hxx
