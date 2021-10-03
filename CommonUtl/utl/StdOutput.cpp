
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
	void __declspec(noreturn) ThrowWriteToConsole( size_t charCount ) throws_( CRuntimeException )
	{
		throw CRuntimeException( str::Format( "Error writing %d characters to console", charCount ) );
	}

	void __declspec(noreturn) ThrowWriteToFile( const fs::CPath& filePath, size_t charCount ) throws_( CRuntimeException )
	{
		throw CRuntimeException( str::Format( _T("Error writing %d characters to redirected stdout file:  %s"), charCount, filePath.GetPtr() ) );
	}
}


namespace io
{
	struct CConsoleWriter : public IWriter
	{
		CConsoleWriter( HANDLE hConsoleOutput ) : m_hConsoleOutput( hConsoleOutput ), m_writtenChars( 0 ) { ASSERT_PTR( m_hConsoleOutput ); }

		virtual void WriteString( const char* pText, size_t charCount ) throws_( CRuntimeException )
		{
			if ( !::WriteConsoleA( m_hConsoleOutput, (const void*)pText, static_cast<DWORD>( charCount ), &m_writtenChars, NULL ) )
				io::ThrowWriteToConsole( charCount );
		}

		virtual void WriteString( const wchar_t* pText, size_t charCount ) throws_( CRuntimeException )
		{
			if ( !::WriteConsoleW( m_hConsoleOutput, (const void*)pText, static_cast<DWORD>( charCount ), &m_writtenChars, NULL ) )
				io::ThrowWriteToConsole( charCount );
		}
	public:
		HANDLE m_hConsoleOutput;
		DWORD m_writtenChars;
	};


	struct CFileWriter : public IWriter
	{
		CFileWriter( HANDLE hFile, const fs::CPath& filePath ) : m_hFile( hFile ), m_filePath( filePath ), m_writtenBytes( 0 ) { ASSERT_PTR( m_hFile ); }

		virtual void WriteString( const char* pText, size_t charCount ) throws_( CRuntimeException )
		{
			if ( !::WriteFile( m_hFile, pText, static_cast<DWORD>( charCount ), &m_writtenBytes, NULL ) )
				io::ThrowWriteToFile( m_filePath, charCount );
		}

		virtual void WriteString( const wchar_t* pWideText, size_t charCount ) throws_( CRuntimeException )
		{
			if ( !::WriteFile( m_hFile, pWideText, static_cast<DWORD>( charCount * sizeof(wchar_t) ), &m_writtenBytes, NULL ) )
				io::ThrowWriteToFile( m_filePath, charCount );
		}
	public:
		HANDLE m_hFile;
		const fs::CPath& m_filePath;
		DWORD m_writtenBytes;
	};
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

	bool CStdOutput::Flush( void )
	{
		return
			IsValid() &&
			StoreLastError( ::FlushFileBuffers( m_hStdOutput ) != FALSE );
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
	void CStdOutput::WriteToConsole( const CharT* pText, size_t charCount ) throws_( CRuntimeException )
	{
		size_t totalWrittenChars = 0;
		io::CConsoleWriter writer( m_hStdOutput );

		for ( size_t leftCount = charCount; leftCount != 0; )
		{
			size_t batchCharCount = std::min( leftCount, s_maxBatchSize );

			writer.WriteString( pText, batchCharCount );

			pText += writer.m_writtenChars;
			leftCount -= writer.m_writtenChars;
			totalWrittenChars += writer.m_writtenChars;
		}
		ENSURE( totalWrittenChars == charCount );
	}

	template< typename CharT >
	void CStdOutput::WriteToFile( const CharT* pText, size_t charCount, fs::Encoding fileEncoding ) throws_( CRuntimeException )
	{
		io::CFileWriter writer( m_hStdOutput, m_fileRedirectPath );
		io::WriteEncodedContents( &writer, fileEncoding, pText, charCount );
	}
}
