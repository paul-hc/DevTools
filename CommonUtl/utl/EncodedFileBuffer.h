#ifndef EncodedFileBuffer_h
#define EncodedFileBuffer_h
#pragma once

#include <fstream>
#include "Path.h"
#include "TextEncoding.h"
#include "RuntimeException.h"


namespace io
{
	abstract class CBaseEncodedBuffer : public utl::IMemoryManaged
	{
	protected:
		CBaseEncodedBuffer( fs::Encoding encoding )
			: m_bom( encoding )
			, m_pCodec( io::MakeCharCodec( encoding ) )
			, m_openMode( 0 )
		{
		}
	public:
		fs::Encoding GetEncoding( void ) const { return m_bom.GetEncoding(); }
		const io::ICharCodec* GetCodec( void ) const { return m_pCodec.get(); }

		bool IsInput( void ) const { return HasFlag( m_openMode, std::ios_base::in ); }
		bool IsOutput( void ) const { return HasFlag( m_openMode, std::ios_base::out ); }
		bool IsBinary( void ) const { return HasFlag( m_openMode, std::ios_base::binary ); }
	protected:
		const fs::CByteOrderMark m_bom;
		std::auto_ptr<ICharCodec> m_pCodec;
		fs::CPath m_filePath;
		std::ios_base::openmode m_openMode;
	};
}


namespace io
{
	// Input/Output file buffer for text files with BOM (Byte Order Mark), with text-mode translation and byte-swapping.
	// Intended to be used directly via the io:: template functions (not via a stream).
	//
	template< typename CharT >
	class CEncodedFileBuffer
		: public std::basic_filebuf<CharT>
		, public CBaseEncodedBuffer
	{
		typedef std::basic_filebuf<CharT> TBaseFileBuf;
		typedef typename TBaseFileBuf::traits_type TTraits;

		using TBaseFileBuf::open;		// hidden
	public:
		CEncodedFileBuffer( fs::Encoding encoding )
			: CBaseEncodedBuffer( encoding )
			, m_lastCh( 0 )
		{
			ASSERT( fs::GetCharByteCount( encoding ) == sizeof( CharT ) );		// design assumption: the file buffer CharT has to match the byte count of the encoding
		}

		virtual ~CEncodedFileBuffer()
		{
			if ( is_open() )
				close();			// necessary since m_buffer goes out of scope if closed by base destructor
		}

		CEncodedFileBuffer* Open( const fs::CPath& filePath, std::ios_base::openmode openMode ) throws_( CRuntimeException )
		{
			bool isOutput = HasFlag( openMode, std::ios_base::out );

			if ( isOutput )
			{
				m_bom.WriteToFile( filePath );								// write the BOM on output before opening
				openMode |= std::ios_base::app;
			}
			if ( GetEncoding() != fs::ANSI_UTF8 && GetEncoding() != fs::UTF8_bom )
				openMode |= std::ios_base::binary;							// force binary mode for wide encodings

			if ( NULL == open( filePath.GetPtr(), openMode ) )
				isOutput ? ThrowOpenForWriting( filePath ) : ThrowOpenForReading( filePath );

			m_filePath = filePath;
			m_openMode = openMode;

			if ( IsBinary() )
			{
				m_buffer.resize( BinaryBufferSize );
				setbuf( &m_buffer[0], m_buffer.size() );					// prevent UTF8 char output conversion: to store wchar_t strings in the buffer
			}
			if ( IsInput() )
			{
				m_itChar = std::istreambuf_iterator< CharT >( this );		// use iterator only for reading
				std::advance( m_itChar, m_bom.GetCharCount() );				// skip the BOM
			}
			return this;
		}

		void close( void )
		{
			__super::close();

			m_filePath.Clear();
			m_openMode = 0;
			m_lastCh = 0;
			m_itChar = std::istreambuf_iterator< CharT >();
		}

		CharT PeekLast( void ) const { return m_lastCh; }


		// reading - strictly file buffer based, not on an istream

		CharT GetNext( void )
		{
			ASSERT( IsInput() );

			CharT chr = 0;

			if ( m_itChar != m_itEnd )
				chr = m_pCodec->Decode( *m_itChar++ );
			else
				ASSERT( false );		// at end?

			m_lastCh = chr;
			return chr;
		}

		bool GetString( std::basic_string< CharT >& rText, CharT delim = 0 )
		{
			rText.clear();

			for ( ;; )
			{
				if ( m_itChar == m_itEnd )
				{	// reached EOF
					if ( !rText.empty() )
						return true;			// read at least a character
					if ( delim != 0 && PopLast() == delim )	// pop to avoid infinite GetString() loop
						return true;			// a trailing empty line

					return false;				// EOF, no content
				}

				CharT chr = GetNext();

				if ( chr == delim )
					return true;				// cut before the delimiter; current position after it
				else if ( chr != '\r' )			// ignore '\r' to simulate text-mode behavior: translate output "\r\n" sequence -> "\n"
					rText.push_back( chr );
			}
		}

		bool GetLine( std::basic_string< CharT >& rLine ) { return GetString( rLine, '\n' ); }

		CharT PushLast( CharT lastCh )
		{
			m_lastCh = lastCh;
			return m_lastCh;
		}

		CharT PopLast( void )
		{
			CharT lastCh = m_lastCh;
			m_lastCh = 0;
			return lastCh;
		}


		// writing - strictly file buffer based, not on an ostream

		virtual void Append( CharT chr ) throws_( CRuntimeException )
		{
			if ( IsBinary() && '\n' == chr )
				if ( 0 == m_lastCh || m_lastCh != '\r' )	// not an "\r\n" explicit sequence?
					PutChar( '\r' );			// translate sequence "\n" -> "\r\n"

			PutChar( chr );
			m_lastCh = chr;
		}

		virtual void Append( const CharT* pText, size_t count = utl::npos ) throws_( CRuntimeException )
		{
			ASSERT_PTR( pText );

			while ( *pText != 0 && count-- != 0 )
				Append( *pText++ );
		}

		void AppendString( const std::basic_string< CharT >& text ) { Append( text.c_str(), text.length() ); }
	private:
		void PutChar( CharT chr ) throws_( CRuntimeException )
		{
			ASSERT( IsOutput() );

			if ( TTraits::eq_int_type( TTraits::eof(), __super::sputc( m_pCodec->Encode( chr ) ) ) )
				throw CRuntimeException( str::Format( _T("Error writing character '%c' to text file %s"), chr, m_filePath.GetPtr() ) );
		}
	private:
		std::vector< CharT > m_buffer;							// set to override the locale-specific conversion in the base

		std::istreambuf_iterator< CharT > m_itChar, m_itEnd;	// read-mode iterators
		CharT m_lastCh;
	};


	// Stream version of encoded file buffer for text files with BOM (Byte Order Mark).
	// Limited use via io::GetLine() function with an istream; problematic due to having to pick the CharT matching the encoding.
	//
	template< typename CharT >
	class CEncodedStreamFileBuffer
		: public CEncodedFileBuffer<CharT>
	{
	public:
		explicit CEncodedStreamFileBuffer( std::basic_istream<CharT>& is, fs::Encoding encoding )
			: CEncodedFileBuffer( encoding )
			, m_pInStream( &is )
			, m_pOutStream( NULL )
			, m_pOldStreamBuff( m_pInStream->rdbuf( this ) )
		{
		}

		explicit CEncodedStreamFileBuffer( std::ostream& os, fs::Encoding encoding )
			: CEncodedFileBuffer( encoding )
			, m_pInStream( NULL )
			, m_pOutStream( &os )
			, m_pOldStreamBuff( m_pOutStream->rdbuf( this ) )
		{
		}

		explicit CEncodedStreamFileBuffer( std::basic_iostream<CharT>& ios, fs::Encoding encoding )
			: CEncodedFileBuffer( encoding )
			, m_pInStream( &ios )
			, m_pOutStream( NULL )
			, m_pOldStreamBuff( m_pInStream->rdbuf( this ) )
		{
		}

		virtual ~CEncodedStreamFileBuffer()
		{
			if ( is_open() )
				close();			// necessary since base m_buffer goes out of scope if closed by base destructor

			m_pInStream != NULL && m_pInStream->rdbuf( m_pOldStreamBuff );
			m_pOutStream != NULL && m_pOutStream->rdbuf( m_pOldStreamBuff );
		}

		CEncodedStreamFileBuffer* Reopen( void ) throws_( CRuntimeException )
		{
			ASSERT_PTR( m_pInStream );		// works only with input streams
			if ( !is_open() )
				return NULL;

			fs::CPath filePath = m_filePath;
			std::ios_base::openmode openMode = m_openMode;
			close();

			__super::Open( filePath, openMode );
			m_pInStream->clear();		// reset the stream state so that it can be read again
			return this;
		}
	private:
		std::basic_istream<CharT>* m_pInStream;
		std::basic_ostream<CharT>* m_pOutStream;
		std::basic_streambuf<CharT>* m_pOldStreamBuff;
	};
}


#endif // EncodedFileBuffer_h
