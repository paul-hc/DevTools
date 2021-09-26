#ifndef TextEncodedFileStreams_h
#define TextEncodedFileStreams_h
#pragma once

#include "Path.h"
#include "TextEncoding.h"


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


	template< typename istream_T >
	size_t GetStreamSize( istream_T& is )
	{
		ASSERT( is.is_open() );

		typename istream_T::pos_type currPos = is.tellg();

		is.seekg( 0, std::ios::end );
		size_t streamCount = is.tellg();

		is.seekg( currPos );			// restore original reading position
		return streamCount;
	}
}


namespace io
{
	// Interface that provides a tight (restricted) API for reading encoded text files.
	//
	template< typename CharT >
	interface ITight_istream : public utl::IMemoryManaged
	{
		typedef std::basic_ifstream< CharT > TBaseStream;

		virtual TBaseStream& GetStream( void ) = 0;			// for text-mode standard output: use it with care (just for ANSI/UTF8 encodings)
		virtual CharT GetNext( void ) = 0;
		virtual void GetStr( std::basic_string< CharT >& rText, CharT delim = 0 ) = 0;

		void GetLine( std::basic_string< CharT >& rLine )
		{
			GetStr( rLine, '\n' );
		}
	};

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
}


namespace io
{
	enum { BinaryBufferSize = 512 };


	// Input stream for text files with BOM (Byte Order Mark) support that opens for reading in binary/append mode.
	// The public API is very restricted to limit support for unformated char/string input with text-mode translation and byte-swapping.
	//
	template< typename CharT, fs::Encoding encoding >
	class CEncoded_ifstream
		: protected std::basic_ifstream< CharT >
		, public ITight_istream< CharT >
	{
	public:
		CEncoded_ifstream( void )
			: m_bom( encoding )
			, m_isBinary( true )
			, m_streamCount( 0 )
			, m_lastCh( 0 )
		{
		}

		CEncoded_ifstream( const fs::CPath& filePath ) throws_( CRuntimeException )
			: m_bom( encoding )
			, m_isBinary( true )
			, m_streamCount( 0 )
			, m_lastCh( 0 )
		{
			Open( filePath );
		}

		~CEncoded_ifstream() { close(); }		// close before m_readBuffer goes out of scope!

		void Open( const fs::CPath& filePath ) throws_( CRuntimeException )
		{
			m_filePath = filePath;
			m_lastCh = 0;

			if ( fs::ANSI == encoding || fs::UTF8_bom == encoding )
			{
				// open in text mode (no translation/byte-swapping required)
				open( filePath.GetPtr(), std::ios::in, (int)std::ios::_Openprot );
				m_isBinary = false;
			}
			else
			{
				// open in binary mode (with translation and byte-swapping)
				open( m_filePath.GetPtr(), std::ios::in | std::ios::binary, std::ios::_Openprot );
				m_isBinary = true;

				m_readBuffer.resize( BinaryBufferSize );
				rdbuf()->pubsetbuf( &m_readBuffer[0], m_readBuffer.size() );			// prevent UTF8 char output conversion: to store wchar_t strings in the buffer
			}
			io::CheckOpenForWriting( *this, m_filePath );

			size_t bomCount = m_bom.GetCharCount();
			m_streamCount = io::GetStreamSize( (TBaseStream&)*this ) - bomCount;

			m_itChar = std::istreambuf_iterator< CharT >( *this );
			std::advance( m_itChar, bomCount );			// skip the BOM
		}

		bool AtEnd( void ) const { return m_itChar == m_itEnd; }
		CharT PeekLast( void ) const { return m_lastCh; }

		// ITight_istream<CharT> interface
		virtual TBaseStream& GetStream( void ) { ASSERT( !m_isBinary ); return *this; }		// for text-mode standard output

		virtual CharT GetNext( void )
		{
			ASSERT( is_open() );
			ASSERT( m_itChar != m_itEnd );

			CharT chr = 0;

			if ( !AtEnd() )
				chr = m_decodeFunc( *m_itChar++ );
			else
				ASSERT( false );		// at end?

			m_lastCh = chr;
			return chr;
		}

		virtual void GetStr( std::basic_string< CharT >& rText, CharT delim = 0 )
		{
			rText.clear();
			rText.reserve( m_streamCount );

			while ( m_itChar != m_itEnd )
			{
				CharT lastCh = m_lastCh;		// store before incrementing
				CharT chr = GetNext();

				if ( m_isBinary && '\n' == chr )
					if ( '\r' == lastCh )
					{	// translate output "\r\n" sequence -> "\n"
						size_t lastPos = rText.size() - 1;
						if ( chr == delim )
						{
							rText.erase( rText.begin() + lastPos, rText.end() );	// cut trailing '\r'
							return;				// cut before the delimiter; current position after it
						}
						rText[ lastPos ] = chr;
						continue;
					}

				if ( chr == delim )
					return;						// cut before the delimiter; current position after it

				rText.push_back( chr );
			}
		}
	private:
		fs::CPath m_filePath;				// destination file path
		const fs::CByteOrderMark m_bom;
		bool m_isBinary;					// only when open in binary mode
		std::vector< CharT > m_readBuffer;
		func::CharConvert< encoding > m_decodeFunc;

		size_t m_streamCount;
		std::istreambuf_iterator< CharT > m_itChar, m_itEnd;
		CharT m_lastCh;
	public:
		// from basic_ifstream
		using TBaseStream::is_open;
		using TBaseStream::close;
		// from basic_istream
		using TBaseStream::tellg;
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


	// File output stream with BOM (Byte Order Mark) support that opens for writing in binary/append mode.
	// The public API is very restricted to limit support for unformated char/string output with text-mode translation and byte-swapping.
	//
	template< typename CharT, fs::Encoding encoding >
	class CEncoded_ofstream
		: protected std::basic_ofstream< CharT >
		, public ITight_ostream< CharT >
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

		~CEncoded_ofstream() { close(); }		// close before m_writeBuffer goes out of scope!

		void Open( const fs::CPath& filePath ) throws_( CRuntimeException )
		{
			m_filePath = filePath;
			m_lastCh = 0;
			m_bom.WriteToFile( m_filePath );

			if ( fs::ANSI == encoding || fs::UTF8_bom == encoding )
			{	// open in text/append mode (no translation required)
				open( filePath.GetPtr(), std::ios::out | std::ios::app, (int)std::ios::_Openprot );
				io::CheckOpenForReading( *this, m_filePath );
				m_isBinary = false;
			}
			else
			{	// open in binary/append mode (with translation and byte-swapping)
				open( m_filePath.GetPtr(), std::ios::out | std::ios::app | std::ios::binary, std::ios::_Openprot );
				io::CheckOpenForReading( *this, m_filePath );
				m_isBinary = true;

				m_writeBuffer.resize( BinaryBufferSize );
				rdbuf()->pubsetbuf( &m_writeBuffer[0], m_writeBuffer.size() );			// prevent UTF8 char output conversion: to store wchar_t strings in the buffer
			}
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
		std::vector< CharT > m_writeBuffer;
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


#endif // TextEncodedFileStreams_h
