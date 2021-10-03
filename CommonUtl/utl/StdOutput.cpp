
#include "stdafx.h"
#include "StdOutput.h"
#include "TextEncoding.h"
#include "Endianness.h"
#include "TextEncodedFileStreams.h"
#include "RuntimeException.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace io
{
	void __declspec( noreturn ) ThrowWriteConsole( size_t charCount ) throws_( CRuntimeException )
	{
		throw CRuntimeException( str::Format( "Error writing %d characters to console", charCount ) );
	}

	void __declspec( noreturn ) ThrowWriteFile( const fs::CPath& filePath, size_t charCount ) throws_( CRuntimeException )
	{
		throw CRuntimeException( str::Format( _T("Error writing %d characters to redirected stdout file:  %s"), charCount, filePath.GetPtr() ) );
	}


	template< typename CharT >
	inline const CharT* SkipLineEnd( const CharT* pText )
	{
		ASSERT_PTR( pText );

		if ( '\r' == *pText )
			++pText;

		if ( '\n' == *pText )
			++pText;

		return pText;
	}
}


namespace io
{
	struct CConsoleWriter
	{
		CConsoleWriter( HANDLE hConsoleOutput ) : m_hConsoleOutput( hConsoleOutput ), m_writtenChars( 0 ) { ASSERT_PTR( m_hConsoleOutput ); }

		void Write( const void* pText, size_t charCount ) throws_( CRuntimeException )
		{
			if ( !::WriteConsoleA( m_hConsoleOutput, (const void*)pText, static_cast<DWORD>( charCount ), &m_writtenChars, NULL ) )
				io::ThrowWriteConsole( charCount );
		}

		void Write( const wchar_t* pText, size_t charCount ) throws_( CRuntimeException )
		{
			if ( !::WriteConsoleW( m_hConsoleOutput, (const void*)pText, static_cast<DWORD>( charCount ), &m_writtenChars, NULL ) )
				io::ThrowWriteConsole( charCount );
		}
	public:
		HANDLE m_hConsoleOutput;
		DWORD m_writtenChars;
	};


	struct CFileWriter
	{
		CFileWriter( HANDLE hFile, const fs::CPath& filePath ) : m_hFile( hFile ), m_filePath( filePath ), m_writtenBytes( 0 ) { ASSERT_PTR( m_hFile ); }

		void Write( const char* pText, size_t charCount ) throws_( CRuntimeException )
		{
			if ( !::WriteFile( m_hFile, pText, static_cast<DWORD>( charCount ), &m_writtenBytes, NULL ) )
				io::ThrowWriteFile( m_filePath, charCount );
		}

		void Write( const wchar_t* pWideText, size_t charCount ) throws_( CRuntimeException )
		{
			if ( !::WriteFile( m_hFile, pWideText, static_cast<DWORD>( charCount * sizeof(wchar_t) ), &m_writtenBytes, NULL ) )
				io::ThrowWriteFile( m_filePath, charCount );
		}
	public:
		HANDLE m_hFile;
		const fs::CPath& m_filePath;
		DWORD m_writtenBytes;
	};
}


namespace io
{
	interface IFileEncoder : public utl::IMemoryManaged
	{
		virtual void Write( const char* pText, size_t length ) = 0;
		virtual void Write( const wchar_t* pText, size_t length ) = 0;
	};


	struct CStraightFileEncoder : public IFileEncoder
	{
		CStraightFileEncoder( io::CFileWriter* pWriteFunc ) : m_pWriteFunc( pWriteFunc ) { ASSERT_PTR( m_pWriteFunc ); }

		virtual void Write( const char* pText, size_t length )
		{
			m_pWriteFunc->Write( pText, length );
		}

		virtual void Write( const wchar_t* pText, size_t length )
		{
			m_pWriteFunc->Write( pText, length );
		}
	private:
		io::CFileWriter* m_pWriteFunc;
	};


	struct CSwapBytesFileEncoder : public CStraightFileEncoder
	{
		CSwapBytesFileEncoder( io::CFileWriter* pWriteFunc ) : CStraightFileEncoder( pWriteFunc ) {}

		virtual void Write( const wchar_t* pText, size_t length )
		{	// make a copy, and swap bytes
			if ( length != 0 )
			{
				m_buffer.assign( pText, pText + length );
				std::for_each( m_buffer.begin(), m_buffer.end(), func::SwapBytes<endian::Little, endian::Big>() );
				__super::Write( &m_buffer.front(), length );
			}
		}
	private:
		std::vector< wchar_t > m_buffer;		// excluding the EOS
	};

	struct CUtf8Encoder : public CStraightFileEncoder
	{
		CUtf8Encoder( io::CFileWriter* pWriteFunc ) : CStraightFileEncoder( pWriteFunc ) {}

		virtual void Write( const wchar_t* pText, size_t length )
		{	// make a UTF8 copy conversion
			if ( length != 0 )
			{
				m_utf8 = str::ToUtf8( std::wstring( pText, pText + length ).c_str() );
				__super::Write( &m_utf8[ 0 ], m_utf8.size() );
			}
		}
	private:
		std::string m_utf8;		// excluding the EOS
	};


	IFileEncoder* MakeFileEncoder( io::CFileWriter* pWriteFunc, fs::Encoding fileEncoding )
	{
		switch ( fileEncoding )
		{
			case fs::ANSI_UTF8:
			case fs::UTF8_bom:
				return new CUtf8Encoder( pWriteFunc );
			case fs::UTF16_LE_bom:
				return new CStraightFileEncoder( pWriteFunc );
			case fs::UTF16_be_bom:
				return new CSwapBytesFileEncoder( pWriteFunc );
			case fs::UTF32_LE_bom:
			case fs::UTF32_be_bom:
			default:
				io::ThrowUnsupportedEncoding( fileEncoding );
		}
	}
}


namespace io
{
	size_t CStdOutput::s_maxBatchSize = 8 * KiloByte;		// larger than 8 KB there are diminishing returns (even 4 KB is comparable)

	CStdOutput::CStdOutput( DWORD stdHandle /*= STD_OUTPUT_HANDLE*/ )
		: m_hStdOutput( NULL )
		, m_fileType( 0 )
		, m_isConsoleOutput( false )
		, m_outputMode( OtherOutput )
		, m_lastErrorCode( 0 )
	{
		m_hStdOutput = ::GetStdHandle( stdHandle );

		if ( IsValid() )
		{
			m_fileType = ::GetFileType( m_hStdOutput );
			if ( FILE_TYPE_UNKNOWN == m_fileType )
				StoreLastError();

			switch ( m_fileType & ~FILE_TYPE_REMOTE )
			{
				case FILE_TYPE_CHAR:
				{
					DWORD consoleMode;
					if ( StoreLastError( ::GetConsoleMode( m_hStdOutput, &consoleMode ) != FALSE ) )
					{
						m_isConsoleOutput = true;
						m_outputMode = ConsoleOutput;
					}
					break;
				}
				case FILE_TYPE_DISK:
				{
					m_outputMode = FileRedirectedOutput;

					TCHAR pathBuffer[ MAX_PATH * 2 ];
					if ( StoreLastError( ::GetFinalPathNameByHandle( m_hStdOutput, pathBuffer, COUNT_OF( pathBuffer ), FILE_NAME_OPENED /*FILE_NAME_NORMALIZED*/ ) != 0 ) )
						m_fileRedirectPath.Set( path::SkipHugePrefix( pathBuffer ) );
					break;
				}
			}
		}
		else
			StoreLastError();
	}

	CStdOutput::~CStdOutput()
	{
	}

	bool CStdOutput::StoreLastError( bool exprResult /*= false*/ )
	{
		if ( !exprResult )
			m_lastErrorCode = ::GetLastError();
		return exprResult;
	}

	void CStdOutput::Write( const std::string& allText, fs::Encoding fileEncoding /*= fs::UTF8_bom*/ ) throws_( CRuntimeException )
	{
		REQUIRE( IsValid() );

		if ( m_isConsoleOutput )
			WriteToConsole( allText.c_str(), allText.length() );
		else
			WriteToFile( allText.c_str(), allText.length(), fileEncoding );		// stdout has been redirected to a file or some other device.
	}

	void CStdOutput::Write( const std::wstring& allText, fs::Encoding fileEncoding /*= fs::UTF16_LE_bom*/ ) throws_( CRuntimeException )
	{
		REQUIRE( IsValid() );

		if ( m_isConsoleOutput )
			WriteToConsole( allText.c_str(), allText.length() );
		else	// stdout has been redirected to a file or some other device.
			WriteToFile( allText.c_str(), allText.length(), fileEncoding );
	}


	// template methods

	template< typename CharT >
	void CStdOutput::WriteToConsole( const CharT* pText, size_t length ) throws_( CRuntimeException )
	{
		size_t totalWrittenChars = 0;
		io::CConsoleWriter writer( m_hStdOutput );

		for ( size_t leftCount = length; leftCount != 0; )
		{
			size_t batchCharCount = std::min( leftCount, s_maxBatchSize );

			writer.Write( pText, batchCharCount );

			pText += writer.m_writtenChars;
			leftCount -= writer.m_writtenChars;
			totalWrittenChars += writer.m_writtenChars;
		}
		ENSURE( totalWrittenChars == length );
	}

	template< typename CharT >
	void CStdOutput::WriteToFile( const CharT* pText, size_t length, fs::Encoding fileEncoding ) throws_( CRuntimeException )
	{
		fs::CByteOrderMark bom( fileEncoding );
		io::CFileWriter writer( m_hStdOutput, m_fileRedirectPath );

		if ( !bom.IsEmpty() )
			writer.Write( &bom.Get().front(), bom.Get().size() );		// write the BOM (Byte Order Mark)

		std::auto_ptr<io::IFileEncoder> pFileEncoder( io::MakeFileEncoder( &writer, fileEncoding ) );		// handles the line-end translation and byte swapping

		// translate text to binary mode
		const CharT eol[] = { '\r', '\n', 0 };
		size_t lineCount = 0;

		for ( const CharT* pEnd = pText + length; pText != pEnd; ++lineCount )
		{
			const CharT* pEol = str::FindTokenEnd( pText, eol );

			ASSERT_PTR( pEol );
			pEol = std::min( pEol, pEnd );			// limit to specified length

			size_t lineLength = std::distance( pText, pEol );

			pFileEncoder->Write( pText, lineLength );

			pText = pEol;
			if ( pText != pEnd )					// not the last line?
			{
				pFileEncoder->Write( eol, COUNT_OF( eol ) - 1 );		// write the translated line-end (exclude the EOS)
				pText = io::SkipLineEnd( pText );	// skip this EOL
			}
		}
	}
}
