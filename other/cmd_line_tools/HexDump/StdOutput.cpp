
#include "stdafx.h"
#include "StdOutput.h"
#include <wincon.h>
#include <sstream>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace path
{
	const std::string& GetHugePrefix( void )
	{
		static const std::string s_hugePrefix( "\\\\?\\" );		// "\\?\" without the C escape sequences
		return s_hugePrefix;
	}

	bool HasHugePrefix( const TCHAR* pPath )
	{
		return 0 == ::strncmp( pPath, GetHugePrefix().c_str(), GetHugePrefix().length() );
	}

	const char* SkipHugePrefix( const char* pPath )
	{
		if ( HasHugePrefix( pPath ) )
			return pPath + GetHugePrefix().length();

		return pPath;
	}
}

namespace io
{
	void __declspec(noreturn) ThrowWriteToConsole( size_t charCount ) throws_( std::runtime_error )
	{
		std::ostringstream os;
		os << "Error writing " << charCount << " characters to console";
		throw std::runtime_error( os.str() );
	}

	void __declspec(noreturn) ThrowWriteToFile( const std::string& filePath, size_t charCount ) throws_( std::runtime_error )
	{
		std::ostringstream os;
		os << "Error writing " << charCount << " characters to redirected stdout file: " << filePath.c_str();
		throw std::runtime_error( os.str() );
	}


	size_t GetFilePointer( HANDLE hFile )
	{
		ASSERT_PTR( hFile );
		DWORD currPos = ::SetFilePointer( hFile, 0, NULL, FILE_CURRENT );
		return currPos != INVALID_SET_FILE_POINTER ? currPos : 0;
	}

	std::string GetFilePath( HANDLE hFile )
	{
		std::string filePath;
		TCHAR pathBuffer[ MAX_PATH * 2 ];

		if ( ::GetFinalPathNameByHandle( hFile, pathBuffer, COUNT_OF( pathBuffer ), FILE_NAME_OPENED ) != 0 )
			filePath = path::SkipHugePrefix( pathBuffer );
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
	typedef size_t TByteSize;
	typedef size_t TCharSize;


	// Does low-level writing to output; implemented by text writers (e.g. console, files, output streams).
	//
	interface IWriter : public utl::IMemoryManaged
	{
		virtual bool IsAppendMode( void ) const = 0;			// appending content to an existing file?

		// return written char count
		virtual TCharSize WriteString( const char* pText, TCharSize charCount ) = 0;
	};


	// output algorithms based on IWriter and ITextEncoder

	TByteSize WriteTranslatedText( io::IWriter* pWriter, const char* pText, TCharSize charCount ) throws_( CRuntimeException )
	{	// translate text to binary mode line by line; returns the number of translated lines
		ASSERT_PTR( pWriter );

		const char eol[] = { '\r', '\n', 0 };		// binary mode line-end
		size_t eolLength = COUNT_OF( eol ) - 1, lineCount = 0;
		TByteSize totalBytes = 0;

		for ( const char* pEnd = pText + charCount; pText != pEnd; ++lineCount )
		{
			const char* pEol = str::FindTokenEnd( pText, eol );

			ASSERT_PTR( pEol );
			pEol = (std::min)( pEol, pEnd );			// limit to specified length

			size_t lineLength = std::distance( pText, pEol );

			totalBytes += pWriter->WriteString( pText, lineLength );

			pText = pEol;
			if ( pText != pEnd )					// not the last line?
			{
				totalBytes += pWriter->WriteString( eol, eolLength );			// write the translated line-end (excluding the EOS)
				pText = str::SkipLineEnd( pText );
			}
		}

		return totalBytes;
	}

	TByteSize WriteContents( io::IWriter* pWriter, const char* pText, TCharSize charCount = utl::npos ) throws_( CRuntimeException )
	{	// writes the entire contents to a file, including the required BOM, also performing line-end translation "\n" -> "\r\n"
		ASSERT_PTR( pWriter );

		TByteSize totalBytes = 0;

		str::SettleLength( charCount, pText );
		totalBytes += io::WriteTranslatedText( pWriter, pText, charCount );
		return totalBytes;
	}

	TCharSize BatchWriteText( io::IWriter* pWriter, size_t batchSize, const char* pText, TCharSize charCount = utl::npos ) throws_( CRuntimeException )
	{	// writes large text to a console-like device that has limited output buffering (i.e. batchSize)
		ASSERT_PTR( pWriter );

		str::SettleLength( charCount, pText );

		TCharSize totalChars = 0;
		size_t totalBatches = 0;

		for ( TCharSize leftCount = charCount; leftCount != 0; ++totalBatches )
		{
			TCharSize batchCharCount = (std::min)( leftCount, batchSize );
			TCharSize writtenChars = pWriter->WriteString( pText, batchCharCount );

			pText += writtenChars;
			leftCount -= writtenChars;
			totalChars += writtenChars;
		}
		ENSURE( totalChars == charCount );
		return totalChars;
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

		virtual TCharSize WriteString( const char* pText, TCharSize charCount ) throws_( std::runtime_error )
		{
			if ( !::WriteConsoleA( m_hConsoleOutput, (const void*)pText, static_cast<DWORD>( charCount ), &m_writtenChars, NULL ) )
				io::ThrowWriteToConsole( charCount );

			return m_writtenChars;
		}

		virtual TCharSize WriteString( const wchar_t* pText, TCharSize charCount ) throws_( std::runtime_error )
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

		virtual TCharSize WriteString( const char* pText, TCharSize charCount ) throws_( std::runtime_error )
		{
			if ( !::WriteFile( m_hFile, pText, static_cast<DWORD>( charCount ), &m_writtenBytes, NULL ) )
				io::ThrowWriteToFile( m_filePath, charCount );

			return m_writtenBytes;
		}

		virtual TCharSize WriteString( const wchar_t* pWideText, TCharSize charCount ) throws_( std::runtime_error )
		{
			if ( !::WriteFile( m_hFile, pWideText, static_cast<DWORD>( charCount * sizeof(wchar_t) ), &m_writtenBytes, NULL ) )
				io::ThrowWriteToFile( m_filePath, charCount );

			return m_writtenBytes / sizeof( wchar_t );
		}
	public:
		HANDLE m_hFile;
		std::string m_filePath;
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

	void CStdOutput::Write( const std::string& allText ) throws_( std::runtime_error )
	{
		REQUIRE( IsValid() );

		if ( m_isConsoleOutput )
			WriteToConsole( allText.c_str(), allText.length() );
		else
			WriteToFile( allText.c_str(), allText.length() );		// stdout has been redirected to a file or some other device.
	}


	void CStdOutput::WriteToConsole( const char* pText, size_t charCount ) throws_( std::runtime_error )
	{
		io::CConsoleWriter writer( m_hStdOutput );
		io::BatchWriteText( &writer, s_maxBatchSize, pText, charCount );
	}

	void CStdOutput::WriteToFile( const char* pText, size_t charCount ) throws_( std::runtime_error )
	{
		io::CFileWriter writer( m_hStdOutput );
		io::WriteContents( &writer, pText, charCount );
	}
}
