
#include "stdafx.h"
#include "TextFileParser.h"
#include <fstream>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


void CTextFileParser::ParseFile( const std::tstring& filePath ) throws_( CRuntimeException )
{
	// assume the text file is encoded in UTF8 (no BOM)
	std::ifstream input( str::ToUtf8( filePath.c_str() ).c_str() );
	if ( !input.is_open() )
		throw CRuntimeException( str::Format( _T("Unable to read text file %s"), filePath.c_str() ) );

	ParseStream( input );
	input.close();
}

void CTextFileParser::ParseStream( std::istream& is )
{
	for ( unsigned int lineNo = 1; !is.eof() && lineNo <= m_maxLineCount; ++lineNo )
	{
		std::string lineUtf8;
		std::getline( is, lineUtf8 );

		std::tstring line = str::FromUtf8( lineUtf8.c_str() );
		if ( NULL == m_pLineParserCallback )
			m_parsedLines.push_back( line );
		else if ( !m_pLineParserCallback->OnParseLine( line, lineNo ) )
			break;
	}
}
