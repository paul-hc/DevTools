#ifndef TextFileUtils_h
#define TextFileUtils_h
#pragma once

#include "RuntimeException.h"


namespace fs { class CPath; }


namespace utl
{
	template< typename StringT >
	void ReadTextLine( std::istream& is, StringT& rLine );

	template< typename StringT >
	void WriteTextLine( std::ostream& os, const StringT& line, size_t* pLinePos = NULL );

	template< typename StringT >
	void WriteText( std::ostream& os, const StringT& text );


	// write container of lines to text file - assume the text file is encoded in UTF8 (no BOM)

	template< typename LinesT >
	void WriteLinesToFile( const fs::CPath& targetFilePath, const LinesT& srcLines ) throws_( CRuntimeException );

	template< typename LinesT >
	void WriteLinesToStream( std::ostream& os, const LinesT& srcLines );


	// write string to UTF8 text file

	template< typename StringT >
	void WriteStringToFile( const fs::CPath& targetFilePath, const StringT& text ) throws_( CRuntimeException );
}


template< typename StringT >
interface ILineParserCallback
{
	virtual bool OnParseLine( const StringT& line, unsigned int lineNo ) = 0;		// return false to stop parsing
};


// assume the text file is encoded in UTF8 (no BOM)
//
template< typename StringT >
class CTextFileParser
{
public:
	CTextFileParser( ILineParserCallback<StringT>* pLineParserCallback = NULL ) : m_pLineParserCallback( pLineParserCallback ), m_maxLineCount( UINT_MAX ) {}

	void SetMaxLineCount( unsigned int maxLineCount ) { m_maxLineCount = maxLineCount; }
	void Clear( void ) { m_parsedLines.clear(); }

	void ParseFile( const fs::CPath& srcFilePath ) throws_( CRuntimeException );
	void ParseStream( std::istream& is );

	bool UseCallback( void ) const { return m_pLineParserCallback != NULL; }

	const std::vector< StringT >& GetParsedLines( void ) const { ASSERT( !UseCallback() ); return m_parsedLines; }
	void SwapParsedLines( std::vector< StringT >& rParsedLines ) { ASSERT( !UseCallback() ); rParsedLines.swap( m_parsedLines ); }
private:
	ILineParserCallback< StringT >* m_pLineParserCallback;
	unsigned int m_maxLineCount;
	std::vector< StringT > m_parsedLines;
};


#endif // TextFileUtils_h
