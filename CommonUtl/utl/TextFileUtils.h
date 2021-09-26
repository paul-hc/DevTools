#ifndef TextFileUtils_h
#define TextFileUtils_h
#pragma once

#include "Encoding.h"
#include "TextEncodedFileStreams.h"


namespace io
{
	namespace bin
	{
		// buffer I/O to/from text file

		void ReadAllFromFile( std::vector< char >& rBuffer, const fs::CPath& srcFilePath ) throws_( CRuntimeException );
		void WriteAllToFile( const fs::CPath& targetFilePath, const std::vector< char >& buffer ) throws_( CRuntimeException );
	}
}


namespace io
{
	// IO for encoded files

	template< typename CharT >
	std::auto_ptr< std::basic_istream<CharT> > OpenEncodedInputStream( fs::Encoding& rEncoding, const fs::CPath& srcFilePath ) throws_( CRuntimeException );
}


namespace io
{
	// string read/write with entire text file

	template< typename StringT >
	fs::Encoding ReadStringFromFile( StringT& rText, const fs::CPath& srcFilePath ) throws_( CRuntimeException );

	template< typename StringT >
	bool WriteStringToFile( const fs::CPath& targetFilePath, const StringT& StringT, fs::Encoding encoding = fs::ANSI ) throws_( CRuntimeException );
}


namespace io
{
	// line I/O

	template< typename StringT >
	void ReadTextLine( std::istream& is, StringT& rLine );

	template< typename StringT >
	void WriteTextLine( std::ostream& os, const StringT& line, size_t* pLinePos = NULL );


	// write container of lines to text file

	template< typename LinesT >
	void WriteLinesToFile( const fs::CPath& targetFilePath, const LinesT& srcLines, fs::Encoding encoding ) throws_( CRuntimeException );


	template< typename LinesT >
	void WriteLinesToFile( const fs::CPath& targetFilePath, const LinesT& srcLines ) throws_( CRuntimeException );

	template< typename LinesT >
	void WriteLinesToStream( std::ostream& os, const LinesT& srcLines );
}


template< typename StringT >
interface ILineParserCallback
{
	virtual bool OnParseLine( const StringT& line, unsigned int lineNo ) = 0;		// return false to stop parsing

	virtual void OnBeginParsing( void ) {}
	virtual void OnEndParsing( void ) {}
};


template< typename StringT >
class CTextFileParser
{
public:
	CTextFileParser( ILineParserCallback<StringT>* pLineParserCallback = NULL ) : m_pLineParserCallback( pLineParserCallback ), m_maxLineCount( UINT_MAX ), m_encoding( fs::ANSI ) {}

	void Clear( void ) { m_encoding = fs::ANSI; m_parsedLines.clear(); }
	void SetMaxLineCount( unsigned int maxLineCount ) { m_maxLineCount = maxLineCount; }

	fs::Encoding GetEncoding( void ) { return m_encoding; }

	fs::Encoding ParseFile( const fs::CPath& srcFilePath ) throws_( CRuntimeException );
	void ParseStream( std::istream& is );

	bool UseCallback( void ) const { return m_pLineParserCallback != NULL; }

	const std::vector< StringT >& GetParsedLines( void ) const { ASSERT( !UseCallback() ); return m_parsedLines; }
	void SwapParsedLines( std::vector< StringT >& rParsedLines ) { ASSERT( !UseCallback() ); rParsedLines.swap( m_parsedLines ); }
private:
	ILineParserCallback< StringT >* m_pLineParserCallback;
	unsigned int m_maxLineCount;

	fs::Encoding m_encoding;
	std::vector< StringT > m_parsedLines;
};


#endif // TextFileUtils_h
