#ifndef TextFileParser_h
#define TextFileParser_h
#pragma once

#include "RuntimeException.h"


interface ILineParserCallback
{
	virtual bool OnParseLine( const std::tstring& line, unsigned int lineNo ) = 0;		// return false to stop parsing
};


// assume the text file is encoded in UTF8 (no BOM)

class CTextFileParser
{
public:
	CTextFileParser( ILineParserCallback* pLineParserCallback = NULL )
		: m_pLineParserCallback( pLineParserCallback ), m_maxLineCount( UINT_MAX ) {}

	void SetMaxLineCount( unsigned int maxLineCount ) { m_maxLineCount = maxLineCount; }

	void ParseFile( const std::tstring& filePath ) throws_( CRuntimeException );
	void ParseStream( std::istream& is );

	bool UseCallback( void ) const { return m_pLineParserCallback != NULL; }
	const std::vector< std::tstring >& GetParsedLines( void ) const { ASSERT( !UseCallback() ); return m_parsedLines; }
	void SwapParsedLines( std::vector< std::tstring >& rParsedLines ) { ASSERT( !UseCallback() ); rParsedLines.swap( m_parsedLines ); }
private:
	ILineParserCallback* m_pLineParserCallback;
	unsigned int m_maxLineCount;
	std::vector< std::tstring > m_parsedLines;
};


#endif // TextFileParser_h
