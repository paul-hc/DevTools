#ifndef TextFileIo_h
#define TextFileIo_h
#pragma once

#include "Encoding.h"
#include "EncodedFileBuffer.h"
#include "Io_fwd.h"


// IO for encoded files


namespace io
{
	// string read/write of entire text file with encoding

	template< typename StringT >
	void WriteStringToFile( const fs::CPath& targetFilePath, const StringT& text, fs::Encoding encoding = fs::ANSI_UTF8 ) throws_( CRuntimeException );

	template< typename StringT >
	fs::Encoding ReadStringFromFile( StringT& rText, const fs::CPath& srcFilePath ) throws_( CRuntimeException );


	// line read/write with encoding

	template< typename LinesT >
	void WriteLinesToFile( const fs::CPath& targetFilePath, const LinesT& lines, fs::Encoding encoding ) throws_( CRuntimeException );

	template< typename LinesT >
	fs::Encoding ReadLinesFromFile( LinesT& rLines, const fs::CPath& srcFilePath ) throws_( CRuntimeException );


	// istream version of reading lines (not the recommended method due to having to guess the CharT matching the encoding)

	template< typename CharT, typename StringT >
	std::basic_istream<CharT>& GetLine( std::basic_istream<CharT>& is, StringT& rLine, CharT delim );

	template< typename CharT, typename StringT >
	inline std::basic_istream<CharT>& GetLine( std::basic_istream<CharT>& is, StringT& rLine ) { return GetLine( is, rLine, is.widen( '\n' ) ); }
}


#include "TextFileIo_fwd.h"		// ILineParserCallback


namespace io
{
	template< typename StringT >
	class CTextFileParser
	{
	public:
		CTextFileParser( ILineParserCallback<StringT>* pLineParserCallback = nullptr ) : m_pLineParserCallback( pLineParserCallback ), m_maxLineCount( UINT_MAX ), m_encoding( fs::ANSI_UTF8 ) {}

		void Clear( void ) { m_parsedLines.clear(); }
		void SetMaxLineCount( unsigned int maxLineCount ) { m_maxLineCount = maxLineCount; }

		fs::Encoding GetEncoding( void ) { return m_encoding; }

		fs::Encoding ParseFile( const fs::CPath& srcFilePath ) throws_( CRuntimeException );

		bool UseCallback( void ) const { return m_pLineParserCallback != nullptr; }

		const std::vector<StringT>& GetParsedLines( void ) const { ASSERT( !UseCallback() ); return m_parsedLines; }
		void SwapParsedLines( std::vector<StringT>& rParsedLines ) { ASSERT( !UseCallback() ); rParsedLines.swap( m_parsedLines ); }
	protected:
		template< typename EncCharT >
		void ParseLinesFromFile( const fs::CPath& srcFilePath ) throws_( CRuntimeException );

		bool PushLine( const StringT& line, size_t lineNo );
	private:
		ILineParserCallback<StringT>* m_pLineParserCallback;
		unsigned int m_maxLineCount;

		fs::Encoding m_encoding;
		std::vector<StringT> m_parsedLines;
	};
}


#endif // TextFileIo_h
