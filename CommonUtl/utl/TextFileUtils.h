#ifndef TextFileUtils_h
#define TextFileUtils_h
#pragma once

#include "RuntimeException.h"
#include "TextEncoding.h"


namespace fs { class CPath; }


namespace io
{
	void __declspec( noreturn ) ThrowOpenForReading( const fs::CPath& filePath ) throws_( CRuntimeException );
	void __declspec( noreturn ) ThrowOpenForWriting( const fs::CPath& filePath ) throws_( CRuntimeException );
	void __declspec( noreturn ) ThrowUnsupportedEncoding( fs::Encoding encoding ) throws_( CRuntimeException );


	template< typename FileStreamT >
	void CheckOpenForReading( FileStreamT& ifs, const fs::CPath& filePath ) throws_( CRuntimeException )
	{
		if ( !ifs.is_open() )
			ThrowOpenForReading( filePath );
	}

	template< typename FileStreamT >
	void CheckOpenForWriting( FileStreamT& ofs, const fs::CPath& filePath ) throws_( CRuntimeException )
	{
		if ( !ofs.is_open() )
			ThrowOpenForWriting( filePath );
	}
}


namespace io
{
	template< typename istream_T >
	size_t GetStreamSize( istream_T& is );


	namespace bin
	{
		// buffer I/O to/from text file

		void ReadAllFromFile( std::vector< char >& rBuffer, const fs::CPath& srcFilePath ) throws_( CRuntimeException );
		void WriteAllToFile( const fs::CPath& targetFilePath, const std::vector< char >& buffer ) throws_( CRuntimeException );
	}
}


namespace io
{
	// Interface that provides a tight (restricted) API for writing encoded text files.
	//
	template< typename CharT >
	interface ITight_ostream : public utl::IMemoryManaged
	{
		typedef std::basic_ofstream< CharT > TBaseStream;

		virtual TBaseStream& GetStream( void ) = 0;			// for text-mode standard output: use it with care (just for ANSI/UTF8 encodings)
		virtual void Append( CharT chr ) = 0;

		virtual void Append( const CharT* pText )
		{
			ASSERT_PTR( pText );
			for ( ; *pText != 0; ++pText )
				Append( *pText );
		}

		virtual void AppendStr( const std::basic_string< CharT >& text ) { Append( text.c_str() ); }
	};


	// File output stream with BOM (Byte Order Mark) support that opens for writing in binary/append mode.
	// The public API is very restricted to limit support for unformated char/string output with text-mode translation and byte-swapping.
	//
	template< typename CharT, fs::Encoding encoding = fs::ANSI >
	class CEncoded_ofstream : protected std::basic_ofstream< CharT >, public ITight_ostream< CharT >
	{
	public:
		CEncoded_ofstream( void )
			: m_bom( encoding )
			, m_isBinary( true )
			, m_lastCh( 0 )
		{
		}

		CEncoded_ofstream( const fs::CPath& filePath ) throws_( CRuntimeException )
			: m_bom( encoding )
			, m_isBinary( true )
			, m_lastCh( 0 )
		{
			Open( filePath );
		}

		CEncoded_ofstream& Open( const fs::CPath& filePath ) throws_( CRuntimeException )
		{
			m_filePath = filePath;
			m_isBinary = true;
			m_lastCh = 0;

			// open in binary mode (with translation and byte-swapping)
			open( m_filePath.GetPtr(), std::ios::out | std::ios::binary, std::ios::_Openprot );
			io::CheckOpenForReading( *this, m_filePath );

			rdbuf()->pubsetbuf( m_writeBuffer, COUNT_OF( m_writeBuffer ) );			// prevent UTF8 char input conversion: to store wchar_t strings in the buffer

			if ( !m_bom.IsEmpty() )
				write( (const CharT*)&m_bom.Get().front(), m_bom.GetCharCount() );	// write the BOM at the beginning of file

			switch ( encoding )
			{
				case fs::ANSI:
				case fs::UTF8_bom:
					// re-open in text/append mode
					close();
					open( filePath.GetPtr(), std::ios::out | std::ios::app, (int)std::ios::_Openprot );	// re-open in text mode (no translation required)
					io::CheckOpenForReading( *this, m_filePath );
					m_isBinary = false;
					break;
			}
			return *this;
		}

		// ITight_ostream<CharT> interface
		virtual TBaseStream& GetStream( void ) { ASSERT( !m_isBinary ); return *this; }		// for text-mode standard output

		virtual void Append( CharT chr )
		{
			ASSERT( is_open() );

			if ( m_isBinary && '\n' == chr )
				if ( 0 == m_lastCh || m_lastCh != '\r' )		// not an "\r\n" explicit sequence?
					put( m_encodeFunc( '\r' ) );				// translate sequence "\n" -> "\r\n"

			put( m_encodeFunc( chr ) );
			m_lastCh = chr;
		}
	private:
		fs::CPath m_filePath;				// destination file path
		const fs::CByteOrderMark m_bom;
		bool m_isBinary;					// only when open in binary mode
		CharT m_writeBuffer[ KiloByte ];
		func::CharConvert< encoding > m_encodeFunc;

		CharT m_lastCh;
	public:
		// from basic_ofstream
		using TBaseStream::is_open;
		using TBaseStream::close;
		// from basic_ostream
		using TBaseStream::flush;
		using TBaseStream::tellp;
		// from basic_ios
		using TBaseStream::imbue;
		// from ios_base
		using TBaseStream::good;
		using TBaseStream::eof;
		using TBaseStream::fail;
		using TBaseStream::bad;
		using TBaseStream::operator void*;
		using TBaseStream::operator!;
	};
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

	fs::Encoding ReadStringFromFile( std::string& rText, const fs::CPath& srcFilePath ) throws_( CRuntimeException );
	fs::Encoding ReadStringFromFile( std::wstring& rText, const fs::CPath& srcFilePath ) throws_( CRuntimeException );
	fs::Encoding ReadStringFromFile( str::wstring4& rText, const fs::CPath& srcFilePath ) throws_( CRuntimeException );


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


#include "Encoding.h"


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
