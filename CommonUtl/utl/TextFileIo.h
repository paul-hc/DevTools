#ifndef TextFileIo_h
#define TextFileIo_h
#pragma once

#include "Encoding.h"
#include "TextEncodedFileStreams.h"


namespace io
{
	namespace bin
	{
		// buffer I/O for binary files

		void ReadAllFromFile( std::vector< char >& rBuffer, const fs::CPath& srcFilePath ) throws_( CRuntimeException );
		void WriteAllToFile( const fs::CPath& targetFilePath, const std::vector< char >& buffer ) throws_( CRuntimeException );
	}
}


// IO for encoded files


namespace io
{
	// string read/write with entire text file

	template< typename StringT >
	fs::Encoding ReadStringFromFile( StringT& rText, const fs::CPath& srcFilePath ) throws_( CRuntimeException );

	template< typename StringT >
	bool WriteStringToFile( const fs::CPath& targetFilePath, const StringT& StringT, fs::Encoding encoding = fs::ANSI_UTF8 ) throws_( CRuntimeException );
}


namespace io
{
	// line I/O with encoding

	template< typename LinesT >
	fs::Encoding ReadLinesFromFile( LinesT& rLines, const fs::CPath& srcFilePath ) throws_( CRuntimeException );

	template< typename LinesT >
	void WriteLinesToFile( const fs::CPath& targetFilePath, const LinesT& srcLines, fs::Encoding encoding = fs::ANSI_UTF8 ) throws_( CRuntimeException );
}


namespace io
{
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
		CTextFileParser( ILineParserCallback<StringT>* pLineParserCallback = NULL ) : m_pLineParserCallback( pLineParserCallback ), m_maxLineCount( UINT_MAX ), m_encoding( fs::ANSI_UTF8 ) {}

		void Clear( void ) { m_encoding = fs::ANSI_UTF8; m_parsedLines.clear(); }
		void SetMaxLineCount( unsigned int maxLineCount ) { m_maxLineCount = maxLineCount; }

		fs::Encoding GetEncoding( void ) { return m_encoding; }

		fs::Encoding ParseFile( const fs::CPath& srcFilePath ) throws_( CRuntimeException );

		template< typename CharT >
		void ParseStream( ITight_istream<CharT>& tis );

		bool UseCallback( void ) const { return m_pLineParserCallback != NULL; }

		const std::vector< StringT >& GetParsedLines( void ) const { ASSERT( !UseCallback() ); return m_parsedLines; }
		void SwapParsedLines( std::vector< StringT >& rParsedLines ) { ASSERT( !UseCallback() ); rParsedLines.swap( m_parsedLines ); }
	protected:
		template< fs::Encoding encoding, typename CharT >
		bool ParseLinesFromFile( const fs::CPath& srcFilePath ) throws_( CRuntimeException );

		bool PushLine( const StringT& line, unsigned int lineNo );
	private:
		ILineParserCallback< StringT >* m_pLineParserCallback;
		unsigned int m_maxLineCount;

		fs::Encoding m_encoding;
		std::vector< StringT > m_parsedLines;
	};
}


#endif // TextFileIo_h
