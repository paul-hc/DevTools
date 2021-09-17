#ifndef TextFileUtils_hxx
#define TextFileUtils_hxx

#include "stdafx.h"
#include "TextFileUtils.h"
#include "Path.h"
#include <fstream>


// line streaming template code

namespace utl
{
	template<>
	void ReadTextLine< std::string >( std::istream& is, std::string& rLineUtf8 )
	{
		std::getline( is, rLineUtf8 );
	}

	template<>
	void ReadTextLine< std::wstring >( std::istream& is, std::wstring& rLineWide )
	{
		std::string lineUtf8;
		std::getline( is, lineUtf8 );

		rLineWide = str::FromUtf8( lineUtf8.c_str() );
	}


	bool WriteLineEnd( std::ostream& os, size_t& rLinePos )
	{	// prepend a line end if not the first line
		if ( 0 == rLinePos++ )
			return false;			// skip newline for the first added line

		os << std::endl;
		return true;
	}


	template<>
	void WriteTextLine< std::string >( std::ostream& os, const std::string& lineUtf8, size_t* pLinePos /*= NULL*/ )
	{
		if ( pLinePos != NULL )
			WriteLineEnd( os, *pLinePos );

		os << lineUtf8.c_str();
	}

	template<>
	void WriteTextLine< std::wstring >( std::ostream& os, const std::wstring& lineWide, size_t* pLinePos /*= NULL*/ )
	{
		if ( pLinePos != NULL )
			WriteLineEnd( os, *pLinePos );

		os << str::ToUtf8( lineWide.c_str() );
	}


	template<>
	void WriteText< std::string >( std::ostream& os, const std::string& textUtf8 )
	{
		os << textUtf8.c_str();
	}


	template<>
	void WriteText< std::wstring >( std::ostream& os, const std::wstring& textWide )
	{
		os << str::ToUtf8( textWide.c_str() );
	}


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


	// write string to UTF8 text file

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

	for ( unsigned int lineNo = 1; !is.eof() && lineNo <= m_maxLineCount; ++lineNo )
	{
		StringT line;

		utl::ReadTextLine( is, line );

		if ( NULL == m_pLineParserCallback )
			m_parsedLines.push_back( line );
		else if ( !m_pLineParserCallback->OnParseLine( line, lineNo ) )
			break;
	}
}


#endif // TextFileUtils_hxx
