#ifndef TextFileUtils_hxx
#define TextFileUtils_hxx

#include "stdafx.h"
#include "TextFileUtils.h"
#include "TextEncoding.h"
#include "Path.h"
#include "Endianness.h"
#include "EnumTags.h"
#include <fstream>


// text streaming template code

namespace io
{
	template< typename istream_T >
	size_t GetStreamSize( istream_T& is )
	{
		ASSERT( is.is_open() );

		typename istream_T::pos_type origPos = is.tellg();

		is.seekg( 0, std::ios::end );
		size_t streamCount = is.tellg();

		is.seekg( origPos, std::ios::beg );			// restore original reading position
		return streamCount;
	}


	template< typename CharT >
	std::auto_ptr< std::basic_istream<CharT> > OpenEncodedInputStream( fs::Encoding& rEncoding, const fs::CPath& srcFilePath ) throws_( CRuntimeException )
	{	// at exit the stream will be positioned at the start of the file content (after BOM, if any)
		fs::CByteOrderMark bom;
		switch ( bom.ReadFromFile( srcFilePath ) )
		{
			case fs::ANSI:
			case fs::UTF8_bom:
			{
				std::basic_ifstream< CharT >* pIfs = new std::basic_ifstream< CharT >( srcFilePath.GetPtr() );
				io::CheckOpenForReading( *pIfs, srcFilePath );
				bom.SeekAfterBom( *pIfs );			// skip the BOM
				return std::auto_ptr< std::basic_istream<CharT> >( pIfs );
			}
			case fs::UTF16_LE_bom:
			case fs::UTF16_be_bom:
			{
				std::basic_string< CharT > text;
				rEncoding = ReadStringFromFile( text, srcFilePath );
				return std::auto_ptr< std::basic_istream<CharT> >( new std::basic_istringstream<CharT>( text, std::ios::in ) );
			}
			default:
				throw CRuntimeException( str::Format( _T("Support for %s text file encoding not implemented"), fs::GetTags_Encoding().FormatUi( bom.GetEncoding() ) ) );
		}
	}
}


namespace io
{
	// write container of lines to text file

	template< typename LinesT >
	void WriteLinesToFile( const fs::CPath& targetFilePath, const LinesT& srcLines ) throws_( CRuntimeException )
	{
		std::ofstream ofs( targetFilePath.GetPtr() );
		io::CheckOpenForWriting( ofs, targetFilePath );

		WriteLinesToStream( ofs, srcLines );		// save all lines
		ofs.close();
	}

	template< typename LinesT >
	void WriteLinesToStream( std::ostream& os, const LinesT& srcLines )
	{
		size_t linePos = 0;
		for ( typename LinesT::const_iterator itLine = srcLines.begin(); itLine != srcLines.end(); ++itLine )
			WriteTextLine( os, *itLine, &linePos );
	}
}


// CTextFileParser template code

template< typename StringT >
fs::Encoding CTextFileParser< StringT >::ParseFile( const fs::CPath& srcFilePath ) throws_( CRuntimeException )
{
	std::auto_ptr< std::istream > pis = io::OpenEncodedInputStream< char >( m_encoding, srcFilePath );		// assume the text file is encoded

	ParseStream( *pis );
	return m_encoding;
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
		io::ReadTextLine( is, line );

		if ( NULL == m_pLineParserCallback )
			m_parsedLines.push_back( line );
		else if ( !m_pLineParserCallback->OnParseLine( line, lineNo ) )
			break;
	}

	if ( m_pLineParserCallback != NULL )
		m_pLineParserCallback->OnEndParsing();
}


#endif // TextFileUtils_hxx
