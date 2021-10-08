#ifndef TextEncodedFileStreams_h
#define TextEncodedFileStreams_h
#pragma once

#include "Path.h"
#include "TextEncoding.h"
#include <fstream>


namespace io
{
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

		is.seekg( 0, std::ios_base::end );
		size_t streamCount = static_cast<size_t>( is.tellg() );

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
		typedef CharT char_type;
		typedef std::basic_ifstream< CharT > TBaseIStream;

		virtual TBaseIStream& GetStream( void ) = 0;		// for text-mode standard output: use it with care (just for ANSI_UTF8 encodings)
		virtual bool AtEnd( void ) const = 0;				// finished iterating?
		virtual CharT PeekLast( void ) const = 0;			// get last read character without incrementing
		virtual CharT GetNext( void ) = 0;
		virtual bool GetStr( std::basic_string< CharT >& rText, CharT delim = 0 ) = 0;		// returns false when EOF reached - i.e. AtEnd() == true

		bool GetLine( std::basic_string< CharT >& rLine )
		{
			return GetStr( rLine, '\n' );
		}
	};

	// Interface that provides a tight (restricted) API for writing encoded text files.
	//
	template< typename CharT >
	interface ITight_ostream : public utl::IMemoryManaged
	{
		typedef CharT char_type;
		typedef std::basic_ofstream< CharT > TBaseOStream;

		virtual TBaseOStream& GetStream( void ) = 0;			// for text-mode standard output: use it with care (just for ANSI_UTF8 encodings)
		virtual void Append( CharT chr ) = 0;
		virtual void Append( const CharT* pText, size_t count = utl::npos ) = 0;

		void AppendStr( const std::basic_string< CharT >& text )
		{
			Append( text.c_str(), text.length() );
		}
	};
}


namespace io
{
	// Input stream for text files with BOM (Byte Order Mark) support that opens for reading in binary/append mode.
	// The public API is very restricted to limit support for unformated char/string input with text-mode translation and byte-swapping.
	// Default implementation is for binary input streams (assumes wchar_t for CharT), with explicit method specializations for text mode char ANSI_UTF8 encoding.
	//

	template< typename CharT >
	abstract class CBase_ifstream
		: protected std::basic_ifstream< CharT >
		, public ITight_istream< CharT >
	{
	protected:
		CBase_ifstream( fs::Encoding encoding )
			: m_bom( encoding )
			, m_lastCh( 0 )
		{
		}

		virtual CharT DecodeChar( CharT chr ) const = 0;
	public:
		void Open( const fs::CPath& filePath ) throws_( CRuntimeException )
		{
			m_filePath = filePath;
			m_lastCh = 0;

			// open in binary mode - with translation and byte-swapping
			open( m_filePath.GetPtr(), std::ios_base::in | std::ios_base::binary, std::ios_base::_Openprot );
			io::CheckOpenForWriting( *this, m_filePath );

			m_readBuffer.resize( BinaryBufferSize );
			rdbuf()->pubsetbuf( &m_readBuffer[0], m_readBuffer.size() );			// prevent UTF8 char output conversion: to store wchar_t strings in the buffer

			Init();
		}

		void close( void )
		{
			__super::close();

			m_filePath.Clear();
			m_lastCh = 0;
			m_streamCount = 0;
			m_itChar = std::istreambuf_iterator< CharT >();
		}

		// ITight_istream<CharT> interface
		virtual TBaseIStream& GetStream( void ) { ASSERT( false ); return *this; }	// allowed only for text-mode standard input

		virtual bool AtEnd( void ) const { return m_itChar == m_itEnd; }
		virtual CharT PeekLast( void ) const { return m_lastCh; }

		virtual CharT GetNext( void )
		{
			ASSERT( is_open() );
			ASSERT( !AtEnd() );

			CharT chr = 0;

			if ( !AtEnd() )
				chr = DecodeChar( *m_itChar++ );
			else
				ASSERT( false );		// at end?

			m_lastCh = chr;
			return chr;
		}

		virtual bool GetStr( std::basic_string< CharT >& rText, CharT delim = 0 )
		{
			rText.clear();
			if ( 0 == delim )
				rText.reserve( m_streamCount );	// allocate space for the entire stream

			for ( ;; )
			{
				if ( AtEnd() )
				{	// reached EOF
					if ( !rText.empty() )
						return true;			// read at least a character
					if ( delim != 0 && m_lastCh == delim )
					{
						m_lastCh = 0;			// avoid infinite GetStr() loop
						return true;			// a trailing empty line
					}
					return false;				// EOF, no content
				}

				CharT chr = GetNext();

				if ( chr == delim )
					return true;				// cut before the delimiter; current position after it
				else if ( chr == '\r' )			// ignore '\r' to simulate text-mode behavior: translate output "\r\n" sequence -> "\n"
					continue;

				rText.push_back( chr );
			}
		}
	private:
		void Init( void )
		{
			size_t bomCount = m_bom.GetCharCount();
			m_streamCount = io::GetStreamSize( (TBaseIStream&)*this ) - bomCount;

			m_itChar = std::istreambuf_iterator< CharT >( *this );
			std::advance( m_itChar, bomCount );			// skip the BOM
		}
	private:
		fs::CPath m_filePath;				// destination file path
		const fs::CByteOrderMark m_bom;
		std::vector< CharT > m_readBuffer;

		size_t m_streamCount;
		std::istreambuf_iterator< CharT > m_itChar, m_itEnd;
		CharT m_lastCh;
	public:
		// from basic_ifstream
		using TBaseIStream::is_open;
		// from basic_istream
		using TBaseIStream::tellg;
		// from basic_ios
		using TBaseIStream::imbue;
		// from ios_base
		using TBaseIStream::good;
		using TBaseIStream::eof;
		using TBaseIStream::fail;
		using TBaseIStream::bad;
		using TBaseIStream::operator!;
	};


	// CBase_ifstream<char> template specialization for reading char-s in text mode - only for ANSI_UTF8 encoding

	template<>
	void CBase_ifstream<char>::Open( const fs::CPath& filePath ) throws_( CRuntimeException )
	{
		m_filePath = filePath;
		m_lastCh = 0;

		// open in text mode - no translation required
		open( filePath.GetPtr(), std::ios_base::in, std::ios_base::_Openprot );
		io::CheckOpenForWriting( *this, m_filePath );

		Init();
	}

	template<>
	CBase_ifstream<char>::TBaseIStream& CBase_ifstream<char>::GetStream( void )		// only for text-mode standard output
	{
		return *this;
	}

	template<>
	char CBase_ifstream<char>::GetNext( void )
	{
		ASSERT( is_open() );
		ASSERT( !AtEnd() );

		char chr = 0;

		if ( !AtEnd() )
			chr = *m_itChar++;
		else
			ASSERT( false );		// at end?

		m_lastCh = chr;
		return chr;
	}

	template<>
	bool CBase_ifstream<char>::GetStr( std::string& rText, char delim /*= 0*/ )
	{
		rText.clear();
		if ( 0 == delim )
			rText.reserve( m_streamCount );	// allocate space for the entire stream

		for ( ;; )
		{
			if ( AtEnd() )
			{	// reached EOF
				if ( !rText.empty() )
					return true;			// read at least a character
				if ( delim != 0 && m_lastCh == delim )
				{
					m_lastCh = 0;			// avoid infinite GetStr() loop
					return true;			// a trailing empty line
				}
				return false;				// EOF, no content
			}

			char chr = GetNext();

			if ( chr == delim )
				return true;				// cut before the delimiter; current position after it

			ASSERT( chr != '\r' );			// no CR expected in text-mode
			rText.push_back( chr );
		}
	}


	// concrete input stream with character decoding
	//
	template< typename CharT, fs::Encoding encoding >
	class CEncoded_ifstream
		: public CBase_ifstream<CharT>
	{
	public:
		CEncoded_ifstream( void ) : CBase_ifstream<CharT>( encoding ) {}
		CEncoded_ifstream( const fs::CPath& filePath ) throws_( CRuntimeException ) : CBase_ifstream<CharT>( encoding ) { Open( filePath ); }
		~CEncoded_ifstream() { close(); }		// close before m_readBuffer goes out of scope!
	protected:
		// base overrides
		virtual CharT DecodeChar( CharT chr ) const
		{
			return m_decodeFunc( chr );
		}
	private:
		func::CharDecoder< encoding > m_decodeFunc;
	};


	// File output stream with BOM (Byte Order Mark) support that opens for writing in binary/append mode.
	// The public API is very restricted to limit support for unformated char/string output with text-mode translation and byte-swapping.
	// Default implementation is for binary output streams (assumes wchar_t for CharT), with explicit method specializations for text mode char ANSI_UTF8 encoding.
	//

	template< typename CharT >
	abstract class CBase_ofstream
		: protected std::basic_ofstream< CharT >
		, public ITight_ostream< CharT >
	{
	protected:
		CBase_ofstream( fs::Encoding encoding )
			: m_bom( encoding )
			, m_lastCh( 0 )
		{
		}

		virtual CharT EncodeChar( CharT chr ) const = 0;
	public:
		void Open( const fs::CPath& filePath ) throws_( CRuntimeException )
		{
			m_filePath = filePath;
			m_lastCh = 0;
			m_bom.WriteToFile( m_filePath );

			// open in binary/append mode - we handle custom translation and byte-swapping
			open( m_filePath.GetPtr(), std::ios_base::out | std::ios_base::app | std::ios_base::binary, std::ios_base::_Openprot );
			io::CheckOpenForReading( *this, m_filePath );

			m_writeBuffer.resize( BinaryBufferSize );
			rdbuf()->pubsetbuf( &m_writeBuffer[0], m_writeBuffer.size() );			// prevent UTF8 char output conversion: to store wchar_t strings in the buffer
		}

		void close( void )
		{
			__super::close();

			m_filePath.Clear();
			m_lastCh = 0;
		}


		// ITight_ostream<CharT> interface
		virtual TBaseOStream& GetStream( void ) { ASSERT( false ); return *this; }	// allowed only for text-mode standard output

		virtual void Append( CharT chr )
		{
			ASSERT( is_open() );

			if ( '\n' == chr )
				if ( 0 == m_lastCh || m_lastCh != '\r' )	// not an "\r\n" explicit sequence?
					put( EncodeChar( '\r' ) );				// translate sequence "\n" -> "\r\n"

			put( EncodeChar( chr ) );
			m_lastCh = chr;
		}

		virtual void Append( const CharT* pText, size_t count = utl::npos )
		{
			ASSERT_PTR( pText );

			while ( *pText != 0 && count-- != 0 )
				Append( *pText++ );
		}
	private:
		fs::CPath m_filePath;				// destination file path
		const fs::CByteOrderMark m_bom;
		std::vector< CharT > m_writeBuffer;

		CharT m_lastCh;
	public:
		// from basic_ofstream
		using TBaseOStream::is_open;
		// from basic_ostream
		using TBaseOStream::flush;
		using TBaseOStream::tellp;
		// from basic_ios
		using TBaseOStream::imbue;
		// from ios_base
		using TBaseOStream::good;
		using TBaseOStream::eof;
		using TBaseOStream::fail;
		using TBaseOStream::bad;
		using TBaseOStream::operator!;
	};


	// CBase_ofstream template specialization for writing char-s in text mode - only for ANSI_UTF8 encoding

	template<>
	void CBase_ofstream<char>::Open( const fs::CPath& filePath ) throws_( CRuntimeException )
	{
		m_filePath = filePath;
		m_lastCh = 0;
		m_bom.WriteToFile( m_filePath );

		// open in text/append mode (no translation required)
		open( m_filePath.GetPtr(), std::ios_base::out | std::ios_base::app, std::ios_base::_Openprot );
		io::CheckOpenForReading( *this, m_filePath );
	}

	template<>
	CBase_ofstream<char>::TBaseOStream& CBase_ofstream<char>::GetStream( void )
	{	// only for text-mode standard output
		return *this;
	}

	template<>
	void CBase_ofstream<char>::Append( char chr )
	{
		ASSERT( is_open() );
		put( chr );
		m_lastCh = chr;
	}

	template<>
	void CBase_ofstream<char>::Append( const char* pText, size_t count /*= utl::npos*/ )
	{
		ASSERT_PTR( pText );

		if ( utl::npos == count )
			count = str::GetLength( pText );

		if ( *pText != 0 && count != 0 )
		{
			write( pText, count );						// unformatted output
			m_lastCh = pText[ count - 1 ];				// keep track of the last character
		}
	}


	// concrete output stream with character encoding
	//
	template< typename CharT, fs::Encoding encoding >
	class CEncoded_ofstream
		: public CBase_ofstream<CharT>
	{
	public:
		CEncoded_ofstream( void ) : CBase_ofstream<CharT>( encoding ) {}
		CEncoded_ofstream( const fs::CPath& filePath ) throws_( CRuntimeException ) : CBase_ofstream<CharT>( encoding ) { Open( filePath ); }
		~CEncoded_ofstream() { close(); }		// close before m_writeBuffer goes out of scope!
	protected:
		// base overrides
		virtual CharT EncodeChar( CharT chr ) const
		{
			return m_encodeFunc( chr );
		}
	private:
		func::CharEncoder< encoding > m_encodeFunc;
	};
}


#endif // TextEncodedFileStreams_h
