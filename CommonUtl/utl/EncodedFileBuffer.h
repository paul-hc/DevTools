#ifndef EncodedFileBuffer_h
#define EncodedFileBuffer_h
#pragma once

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


#include <fstream>


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


namespace io
{
	namespace impl
	{
		template< typename DestCharT, typename SrcCharT >
		inline void SwapResult( std::basic_string<DestCharT>& rDest, std::basic_string<SrcCharT>& rSrc )
		{
			rDest.swap( rSrc );
		}

		template<>
		inline void SwapResult( std::wstring& rDest, std::string& rSrc )
		{
			std::wstring src = str::FromUtf8( rSrc.c_str() );
			rDest.swap( src );
			rSrc.clear();
		}

		template<>
		inline void SwapResult( std::string& rDest, std::wstring& rSrc )
		{
			std::string src = str::ToUtf8( rSrc.c_str() );
			rDest.swap( src );
			rSrc.clear();
		}
	}


	// istream version of reading lines (not the recommended method due to having to guess the CharT matching the encoding)

	template< typename CharT, typename StringT >
	std::basic_istream<CharT>& GetLine( std::basic_istream<CharT>& is, StringT& rLine, CharT delim )
	{	// get characters into string, discard delimiter
		CEncodedFileBuffer<CharT>* pInBuffer = dynamic_cast< CEncodedFileBuffer<CharT>* >( is.rdbuf() );
		REQUIRE( pInBuffer != NULL );		// to be used only with encoded file buffers; otherwise use std::getline()

		typedef std::basic_istream<CharT> TIStream;
		typedef typename TIStream::traits_type TTraits;
		using std::ios_base;

		ios_base::iostate state = ios_base::goodbit;
		bool changed = false;
		std::basic_string<CharT> line;
		const typename TIStream::sentry ok( is, true );

		if ( ok )
		{	// state okay, extract characters
			const typename TTraits::int_type metaDelim = TTraits::to_int_type( delim ), metaCR = TTraits::to_int_type( '\r' );
			const io::ICharCodec* pDecoder = pInBuffer->GetCodec();

			for ( typename TTraits::int_type metaCh = pDecoder->DecodeMeta<TTraits>( pInBuffer->sgetc() ); ; metaCh = pDecoder->DecodeMeta<TTraits>( pInBuffer->snextc() ) )
				if ( TTraits::eq_int_type( TTraits::eof(), metaCh ) )
				{	// end of file, quit
					state |= ios_base::eofbit;

					if ( !changed )
						if ( delim != 0 && pInBuffer->PopLast() == delim )		// last empty line? pop to avoid infinite GetString() loop
							changed = true;
					break;
				}
				else if ( TTraits::eq_int_type( metaCh, metaDelim ) )
				{	// got a delimiter, discard it and quit
					changed = true;
					pInBuffer->sbumpc();
					pInBuffer->PushLast( delim );
					break;
				}
				else if ( line.size() >= line.max_size() )
				{	// string too large, quit
					state |= ios_base::failbit;
					break;
				}
				else
				{	// got a character, add it to string
					if ( !TTraits::eq_int_type( metaCh, metaCR ) )		// skip the '\r' in binary mode (translate "\r\n" to "\n")
					{
						line += pInBuffer->PushLast( TTraits::to_char_type( metaCh ) );
						changed = true;
					}
				}
		}
		impl::SwapResult( rLine, line );

		if ( !changed )
			state |= ios_base::failbit;

		is.setstate( state );
		return is;
	}


	template< typename CharT, typename StringT >
	inline std::basic_istream<CharT>& GetLine( std::basic_istream<CharT>& is, StringT& rLine )
	{
		return GetLine( is, rLine, is.widen( '\n' ) );
	}
}


namespace io
{
	namespace nt
	{
		// string read/write of entire text file with encoding

		template< typename StringT >
		void WriteStringToFile( const fs::CPath& targetFilePath, const StringT& text, fs::Encoding encoding ) throws_( CRuntimeException );

		template< typename StringT >
		fs::Encoding ReadStringFromFile( StringT& rText, const fs::CPath& srcFilePath ) throws_( CRuntimeException );


		// line read/write with encoding

		template< typename LinesT >
		void WriteLinesToFile( const fs::CPath& targetFilePath, const LinesT& lines, fs::Encoding encoding ) throws_( CRuntimeException );

		template< typename LinesT >
		fs::Encoding ReadLinesFromFile( LinesT& rLines, const fs::CPath& srcFilePath ) throws_( CRuntimeException );


		// write entire string to encoded text file

		namespace impl
		{
			template< typename CharT, typename StringT >
			void AppendText( CEncodedFileBuffer<CharT>& rOutFile, const StringT& text )
			{
				rOutFile.AppendString( text );
			}

			template<>
			inline void AppendText( CEncodedFileBuffer<char>& rOutFile, const std::wstring& text )
			{
				rOutFile.AppendString( str::ToUtf8( text.c_str() ) );
			}

			template<>
			inline void AppendText( CEncodedFileBuffer<wchar_t>& rOutFile, const std::string& text )
			{
				rOutFile.AppendString( str::FromUtf8( text.c_str() ) );
			}


			template< typename CharT, typename StringT >
			void DoWriteStringToFile( const fs::CPath& targetFilePath, const StringT& text, fs::Encoding encoding ) throws_( CRuntimeException )
			{
				CEncodedFileBuffer<CharT> outFile( encoding );
				outFile.Open( targetFilePath, std::ios_base::out );

				AppendText( outFile, text );
			}
		}


		template< typename StringT >
		void WriteStringToFile( const fs::CPath& targetFilePath, const StringT& text, fs::Encoding encoding ) throws_( CRuntimeException )
		{
			switch ( fs::GetCharByteCount( encoding ) )
			{
				case sizeof( char ):
					impl::DoWriteStringToFile<char>( targetFilePath, text, encoding );
					break;
				case sizeof( wchar_t ):
					impl::DoWriteStringToFile<wchar_t>( targetFilePath, text, encoding );
					break;
				default:
					ThrowUnsupportedEncoding( encoding );
			}
		}


		// read entire string from encoded text file

		namespace impl
		{
			template< typename CharT, typename StringT >
			void GetString( CEncodedFileBuffer<CharT>& rInFile, StringT& rText )
			{
				rInFile.GetString( rText );
			}

			template<>
			inline void GetString( CEncodedFileBuffer<char>& rInFile, std::wstring& rText )
			{
				std::string narrowText;
				rInFile.GetString( narrowText );
				rText = str::FromUtf8( narrowText.c_str() );
			}

			template<>
			inline void GetString( CEncodedFileBuffer<wchar_t>& rInFile, std::string& rText )
			{
				std::wstring wideText;
				rInFile.GetString( wideText );
				rText = str::ToUtf8( wideText.c_str() );
			}


			template< typename CharT, typename StringT >
			void DoReadStringFromFile( StringT& rText, const fs::CPath& srcFilePath, fs::Encoding encoding ) throws_( CRuntimeException )
			{
				CEncodedFileBuffer<CharT> inFile( encoding );
				inFile.Open( srcFilePath, std::ios_base::in );

				GetString( inFile, rText );
			}
		}


		template< typename StringT >
		fs::Encoding ReadStringFromFile( StringT& rText, const fs::CPath& srcFilePath ) throws_( CRuntimeException )
		{
			fs::CByteOrderMark bom;
			fs::Encoding encoding = bom.ReadFromFile( srcFilePath );

			switch ( fs::GetCharByteCount( encoding ) )
			{
				case sizeof( char ):
					impl::DoReadStringFromFile<char>( rText, srcFilePath, encoding );
					break;
				case sizeof( wchar_t ):
					impl::DoReadStringFromFile<wchar_t>( rText, srcFilePath, encoding );
					break;
				default:
					ThrowUnsupportedEncoding( encoding );
			}
			return encoding;
		}


		// write container of lines from encoded text file

		namespace impl
		{
			template< typename CharT, typename LinesT >
			void DoWriteLinesToFile( const fs::CPath& targetFilePath, const LinesT& lines, fs::Encoding encoding ) throws_( CRuntimeException )
			{
				CEncodedFileBuffer<CharT> outFile( encoding );
				outFile.Open( targetFilePath, std::ios_base::out );

				size_t pos = 0;
				for ( typename LinesT::const_iterator itLine = lines.begin(), itEnd = lines.end(); itLine != itEnd; ++itLine )
				{
					if ( pos++ != 0 )
						outFile.Append( '\n' );

					AppendText( outFile, *itLine );
				}
			}
		}


		template< typename LinesT >
		void WriteLinesToFile( const fs::CPath& targetFilePath, const LinesT& lines, fs::Encoding encoding ) throws_( CRuntimeException )
		{
			switch ( fs::GetCharByteCount( encoding ) )
			{
				case sizeof( char ):
					impl::DoWriteLinesToFile<char>( targetFilePath, lines, encoding );
					break;
				case sizeof( wchar_t ):
					impl::DoWriteLinesToFile<wchar_t>( targetFilePath, lines, encoding );
					break;
				default:
					ThrowUnsupportedEncoding( encoding );
			}
		}


		// read container of lines from encoded text file

		namespace impl
		{
			template< typename CharT, typename StringT >
			bool GetTextLine( CEncodedFileBuffer<CharT>& rInFile, StringT& rLine )
			{
				return rInFile.GetLine( rLine );
			}

			template<>
			inline bool GetTextLine( CEncodedFileBuffer<char>& rInFile, std::wstring& rLine )
			{
				std::string narrowLine;
				bool noEof = rInFile.GetLine( narrowLine );
				rLine = str::FromUtf8( narrowLine.c_str() );
				return noEof;
			}

			template<>
			inline bool GetTextLine( CEncodedFileBuffer<wchar_t>& rInFile, std::string& rLine )
			{
				std::wstring wideLine;
				bool noEof = rInFile.GetLine( wideLine );
				rLine = str::ToUtf8( wideLine.c_str() );
				return noEof;
			}


			template< typename CharT, typename LinesT >
			void DoReadLinesFromFile( LinesT& rLines, const fs::CPath& srcFilePath, fs::Encoding encoding ) throws_( CRuntimeException )
			{
				typedef typename LinesT::value_type TString;

				CEncodedFileBuffer< CharT > inFile( encoding );
				inFile.Open( srcFilePath, std::ios_base::in );

				rLines.clear();

				for ( TString line; impl::GetTextLine( inFile, line ); )
					rLines.insert( rLines.end(), line );
			}
		}

		template< typename LinesT >
		fs::Encoding ReadLinesFromFile( LinesT& rLines, const fs::CPath& srcFilePath ) throws_( CRuntimeException )
		{
			fs::CByteOrderMark bom;
			fs::Encoding encoding = bom.ReadFromFile( srcFilePath );

			switch ( fs::GetCharByteCount( encoding ) )
			{
				case sizeof( char ):
					impl::DoReadLinesFromFile<char>( rLines, srcFilePath, encoding );
					break;
				case sizeof( wchar_t ):
					impl::DoReadLinesFromFile<wchar_t>( rLines, srcFilePath, encoding );
					break;
				default:
					ThrowUnsupportedEncoding( encoding );
			}
			return bom.GetEncoding();
		}
	}
}


#endif // EncodedFileBuffer_h
