
#include "pch.h"
#include "StdOutput.h"
#include "TextEncoding.h"
#include "Endianness.h"
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


	size_t GetFilePointer( HANDLE hFile )
	{
		ASSERT_PTR( hFile );
		DWORD currPos = ::SetFilePointer( hFile, 0, NULL, FILE_CURRENT );
		return currPos != INVALID_SET_FILE_POINTER ? currPos : 0;
	}

	fs::CPath GetFilePath( HANDLE hFile )
	{
		fs::CPath filePath;
		TCHAR pathBuffer[ MAX_PATH * 2 ];

		if ( ::GetFinalPathNameByHandle( hFile, pathBuffer, COUNT_OF( pathBuffer ), FILE_NAME_OPENED ) != 0 )
			filePath.Set( path::SkipHugePrefix( pathBuffer ) );
		return filePath;
	}

	bool StoreLastError( bool exprResult = false )
	{
		static DWORD s_lastErrorCode = 0;

		if ( !exprResult )
			s_lastErrorCode = ::GetLastError();
		return exprResult;
	}
}


namespace io
{
	struct CConsoleWriter : public IWriter
	{
		CConsoleWriter( HANDLE hConsoleOutput ) : m_hConsoleOutput( hConsoleOutput ), m_writtenChars( 0 ) { ASSERT_PTR( m_hConsoleOutput ); }

		virtual bool IsAppendMode( void ) const
		{
			ASSERT( false );		// shouldn't be called
			return false;
		}

		virtual TCharSize WriteString( const char* pText, TCharSize charCount ) throws_( CRuntimeException )
		{
			if ( !::WriteConsoleA( m_hConsoleOutput, (const void*)pText, static_cast<DWORD>( charCount ), &m_writtenChars, NULL ) )
				io::ThrowWriteToConsole( charCount );

			return m_writtenChars;
		}

		virtual TCharSize WriteString( const wchar_t* pText, TCharSize charCount ) throws_( CRuntimeException )
		{
			if ( !::WriteConsoleW( m_hConsoleOutput, (const void*)pText, static_cast<DWORD>( charCount ), &m_writtenChars, NULL ) )
				io::ThrowWriteToConsole( charCount );

			return m_writtenChars;
		}
	public:
		HANDLE m_hConsoleOutput;
		DWORD m_writtenChars;
	};


	struct CFileWriter : public IWriter
	{
		CFileWriter( HANDLE hFile )
			: m_hFile( hFile )
			, m_filePath( io::GetFilePath( m_hFile ) )
			, m_isAppendMode( io::GetFilePointer( m_hFile ) != 0 )
			, m_writtenBytes( 0 )
		{
			ASSERT_PTR( m_hFile );
		}

		virtual bool IsAppendMode( void ) const
		{
			return m_isAppendMode;
		}

		virtual TCharSize WriteString( const char* pText, TCharSize charCount ) throws_( CRuntimeException )
		{
			if ( !::WriteFile( m_hFile, pText, static_cast<DWORD>( charCount ), &m_writtenBytes, NULL ) )
				io::ThrowWriteToFile( m_filePath, charCount );

			return m_writtenBytes;
		}

		virtual TCharSize WriteString( const wchar_t* pWideText, TCharSize charCount ) throws_( CRuntimeException )
		{
			if ( !::WriteFile( m_hFile, pWideText, static_cast<DWORD>( charCount * sizeof(wchar_t) ), &m_writtenBytes, NULL ) )
				io::ThrowWriteToFile( m_filePath, charCount );

			return m_writtenBytes / sizeof( wchar_t );
		}
	public:
		HANDLE m_hFile;
		fs::CPath m_filePath;
		bool m_isAppendMode;
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
	{
		m_hStdOutput = ::GetStdHandle( stdHandle );

		if ( IsValid() )
		{
			m_fileType = ::GetFileType( m_hStdOutput );
			StoreLastError( FILE_TYPE_UNKNOWN == m_fileType );

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
					m_outputMode = FileRedirectedOutput;
					m_fileRedirectPath = io::GetFilePath( m_hStdOutput );
					break;
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
		io::CConsoleWriter writer( m_hStdOutput );
		io::BatchWriteText( &writer, s_maxBatchSize, pText, charCount );
	}

	template< typename CharT >
	void CStdOutput::WriteToFile( const CharT* pText, size_t charCount, fs::Encoding fileEncoding ) throws_( CRuntimeException )
	{
		io::CFileWriter writer( m_hStdOutput );
		io::WriteEncodedContents( &writer, fileEncoding, pText, charCount );
	}
}
