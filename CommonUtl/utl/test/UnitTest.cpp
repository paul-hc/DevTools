
#include "pch.h"

#ifdef USE_UT		// no UT code in release builds
#include "test/UnitTest.h"
#include "Algorithms.h"
#include "TextFileIo.h"
#include "FileEnumerator.h"
#include "IoBin.h"
#include "Logger.h"
#include "Path.h"
#include "RuntimeException.h"
#include "AppTools.h"
#include "StringUtilities.h"
#include <unordered_set>
#include <fstream>
#include <iostream>
#include <iomanip>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace ut
{
	// UT assertions:

	namespace impl
	{
		std::tstring MakeNotEqualMessage( const std::tstring& expectedValue, const std::tstring& actualValue, const wchar_t* pExpression )
		{
			std::tostringstream os;

			os
				<< L"** Equality Assertion Failed:" << std::endl
				<< L"  " << pExpression << std::endl
				<< std::endl
				<< L"    expected:\t'" << expectedValue << L"'" << std::endl
				<< L"    actual:  \t'" << actualValue << L"'";

			return os.str();
		}

		bool ReportMessage( bool succeeded, const std::tstring& msg, const char* pFilePath, int lineNumber )
		{
			if ( succeeded )
				return true;

		#ifdef IS_CPP_11
			// note: avoid double tracing => _ASSERT_EXPR() will do the tracing
		#else
			TRACE( _T("%s\n"), msg.c_str() );
		#endif

			std::cerr << pFilePath << '(' << lineNumber << ") : " << msg << std::endl;
			CAppTools::AddMainResultError();		// increment error count for console unit tests
			return false;
		}


		template<>
		void TraceMessage<char>( const char* pMessage )
		{
			OutputDebugStringA( pMessage );		// avoids TRACE overhead due to ATL_TRACE
			std::clog << pMessage;
		}

		template<>
		void TraceMessage<wchar_t>( const wchar_t* pMessage )
		{
			OutputDebugStringW( pMessage );		// avoids TRACE overhead due to ATL_TRACE
			std::wclog << pMessage;
		}
	}


	bool AssertTrue( bool succeeded, const wchar_t* pExpression, std::tstring& rMsg )
	{
		if ( succeeded )
			return true;

		std::tostringstream os;

		os
			<< L"** Assertion Failed:" << std::endl
			<< L"  Expression:\t" << pExpression;

		rMsg = os.str();
		return false;
	}
}


namespace ut
{
	bool SetFileText( const fs::CPath& filePath, const TCHAR* pText /*= nullptr*/ )
	{
		if ( !fs::CreateDirPath( filePath.GetParentPath().GetPtr() ) )
			return false;			// error creating sub dir

		if ( nullptr == pText )
			pText = filePath.GetFilenamePtr();		// use the filename as text content

		std::ofstream output( filePath.GetPtr(), std::ios::out | std::ios::trunc );
		output
			<< "Unit-test file: " << std::endl
			<< pText << std::endl;

		return filePath.FileExist();
	}

	bool ModifyFileText( const fs::CPath& filePath, const TCHAR* pText /*= nullptr*/, bool retainModifyTime /*= false*/ )
	{
		if ( !filePath.FileExist() )
		{
			ASSERT( false );
			return false;
		}

		std::auto_ptr<fs::CScopedFileTime> pScopedFileTime( retainModifyTime ? new fs::CScopedFileTime( filePath ) : nullptr );

		if ( nullptr == pText )
			pText = filePath.GetFilenamePtr();		// use the filename as text content

		{
			std::ofstream output( filePath.GetPtr(), std::ios::out | std::ios::app );
			output << pText << std::endl;
		}

		return true;
	}

	void StoreFileTextSize( const fs::CPath& filePath, size_t fileSize )
	{
		std::vector<char> buffer( fileSize, '#' );
		io::bin::WriteAllToFile( filePath, buffer );
	}


	void HexDump( std::ostream& os, const fs::CPath& textPath, size_t rowByteCount /*= DefaultRowByteCount*/ ) throws_( CRuntimeException )
	{	// dump contents of filename to stdout in hex
		std::fstream is( textPath.GetPtr(), std::ios::in | std::ios::binary );
		if ( !is.is_open() )
			io::ThrowOpenForReading( textPath );

		static const char s_unprintableCh = '.';
		static const char s_blankCh = ' ';
		static const char s_columnSep[] = "  ";

		std::vector<char> inputRowBuff( rowByteCount );
		std::string charRow;

		for ( size_t i; !is.eof(); )
		{
			inputRowBuff.assign( rowByteCount, '\0' );

			is.read( &inputRowBuff[ 0 ], rowByteCount );
			inputRowBuff.resize( static_cast<size_t>( is.gcount() ) );		// cut to the actual read chars

			if ( !inputRowBuff.empty() )
			{
				charRow.clear();

				for ( i = 0; i != rowByteCount; ++i )
					if ( i < inputRowBuff.size() )
					{
						char inChar = inputRowBuff[ i ];

						charRow.push_back( inChar >= ' ' ? inChar : s_unprintableCh );
						os << std::setfill('0') << std::setw(2) << std::uppercase << std::hex << (int)(unsigned char)inChar << ' ';
					}
					else
						os << "   ";		// 2 digits + 1 space

				os << s_columnSep << charRow << std::endl;
			}
		}
		is.close();

		/* Sample outputs:
			Hex Dump of wcHello.txt - note that output is ANSI chars:
			48 65 6C 6C 6F 20 57 6F 72 6C 64 00 00 00 00 00   Hello World.....

			Hex Dump of wwHello.txt - note that output is wchar_t chars:
			48 00 65 00 6c 00 6c 00 6f 00 20 00 57 00 6f 00   H.e.l.l.o. .W.o.
			72 00 6c 00 64 00                                 r.l.d.
		*/
	}
}


namespace ut
{
	CLogger& GetTestLogger( void )
	{
		static CLogger s_testLogger( _T("%s_tests") );
		return s_testLogger;
	}



	const fs::TDirPath& GetTestDataDirPath( void ) throws_( CRuntimeException )
	{
		static const fs::TDirPath s_dirPath = env::ExpandStrings( _T("%UTL_TESTDATA_PATH%") );
		if ( !s_dirPath.IsEmpty() && !fs::IsValidDirectory( s_dirPath.GetPtr() ) )
			throw CRuntimeException( str::Format( _T("Cannot find the local test directory path: %s\n\nTODO: define envirnoment variable UTL_TESTDATA_PATH"), s_dirPath.GetPtr() ) );

		return s_dirPath;
	}

	const fs::TDirPath& GetImageSourceDirPath( void )
	{
		static fs::TDirPath s_imagesDirPath = env::ExpandStrings( _T("%UTL_THUMB_SRC_IMAGE_PATH%") );
		if ( !s_imagesDirPath.IsEmpty() && !fs::IsValidDirectory( s_imagesDirPath.GetPtr() ) )
		{
			TRACE( _T("\n # Cannot find unit test images dir path: %s #\nNo environment variable UTL_THUMB_SRC_IMAGE_PATH"), s_imagesDirPath.GetPtr() );
			s_imagesDirPath.Clear();
		}
		return s_imagesDirPath;
	}

	const fs::TDirPath& GetDestImagesDirPath( void )
	{
		static fs::TDirPath s_dirPath = GetTestDataDirPath() / fs::TDirPath( _T("images") );
		if ( !s_dirPath.IsEmpty() && !fs::IsValidDirectory( s_dirPath.GetPtr() ) )
		{
			TRACE( _T("\n * Cannot find the local test images dir path: %s\n"), s_dirPath.GetPtr() );
			s_dirPath.Clear();
		}
		return s_dirPath;
	}

	const fs::TDirPath& GetStdImageDirPath( void )
	{
		static fs::TDirPath s_stdImagesDirPath = GetTestDataDirPath() / fs::TDirPath( _T("std_test_images") );
		if ( !s_stdImagesDirPath.IsEmpty() && !fs::IsValidDirectory( s_stdImagesDirPath.GetPtr() ) )
		{
			TRACE( _T("\n # Cannot find unit test standard images dir path: %s #\nTODO: create directory %UTL_STD_SRC_IMAGE_PATH%\\std_test_images"), s_stdImagesDirPath.GetPtr() );
			s_stdImagesDirPath.Clear();
		}
		return s_stdImagesDirPath;
	}

	const fs::TDirPath& GetStdTestFilesDirPath( void )
	{
		static fs::TDirPath s_stdImagesDirPath = GetTestDataDirPath() / fs::TDirPath( _T("std_test_files") );
		if ( !s_stdImagesDirPath.IsEmpty() && !fs::IsValidDirectory( s_stdImagesDirPath.GetPtr() ) )
		{
			TRACE( _T("\n # Cannot find unit test standard test files dir path: %s #\nTODO: create directory %UTL_STD_SRC_IMAGE_PATH%\\std_test_files"), s_stdImagesDirPath.GetPtr() );
			s_stdImagesDirPath.Clear();
		}
		return s_stdImagesDirPath;
	}


	const fs::TDirPath& GetTempUt_DirPath( void ) throws_( CRuntimeException )
	{
		static fs::TDirPath s_tempUtDirPath;

		if ( s_tempUtDirPath.IsEmpty() )
		{
			s_tempUtDirPath = GetTestDataDirPath() / fs::TDirPath( _T("temp_ut") );
			fs::thr::CreateDirPath( s_tempUtDirPath.GetPtr() );
		}
		return s_tempUtDirPath;
	}

	fs::TDirPath MakeTempUt_DirPath( const fs::TDirPath& subDirPath, bool createDir ) throws_( CRuntimeException )
	{
		fs::TDirPath fullPath = GetTempUt_DirPath() / subDirPath;
		if ( createDir )
			fs::thr::CreateDirPath( fullPath.GetPtr() );
		return fullPath;
	}
}


namespace ut
{
	// CTempFilePool implementation

	const TCHAR CTempFilePool::m_sep[] = _T("|");

	CTempFilePool::CTempFilePool( const TCHAR* pFlatPaths /*= nullptr*/ )
		: m_poolDirPath( MakePoolDirPath( true ) )
		, m_hasFileErrors( false )
	{
		fs::DeleteAllFiles( m_poolDirPath.GetPtr() );				// delete existing files from previously failed tests

		if ( !str::IsEmpty( pFlatPaths ) )
			CreateFiles( pFlatPaths );
	}

	CTempFilePool::~CTempFilePool()
	{
		fs::DeleteDir( m_poolDirPath.GetPtr() );
	}

	fs::TDirPath CTempFilePool::MakePoolDirPath( bool createDir /*= false*/ )
	{
		return MakeTempUt_DirPath( fs::TDirPath( _T("_UT") ), createDir );
	}

	bool CTempFilePool::DeleteAllFiles( void )
	{
		return fs::DeleteAllFiles( m_poolDirPath.GetPtr() );
	}

	bool CTempFilePool::CreateFiles( const TCHAR* pFlatPaths /*= nullptr*/ )
	{
		if ( !IsValidDir() )
			return false;

		m_hasFileErrors = false;

		std::vector<fs::CPath> filePaths;
		str::Split( filePaths, pFlatPaths, m_sep );

		for ( std::vector<fs::CPath>::iterator itSrcPath = filePaths.begin(); itSrcPath != filePaths.end(); ++itSrcPath )
		{
			*itSrcPath = m_poolDirPath / *itSrcPath;		// convert to absolute path
			if ( !ut::SetFileText( *itSrcPath ) )
			{
				m_hasFileErrors = true;
				return false;
			}
		}

		m_filePaths.reserve( m_filePaths.size() + filePaths.size() );
		m_filePaths.insert( m_filePaths.end(), filePaths.begin(), filePaths.end() );

		return true;
	}

	size_t CTempFilePool::SplitQualifyPaths( std::vector<fs::CPath>& rFullPaths, const TCHAR relFilePaths[] ) const
	{
		str::Split( rFullPaths, relFilePaths, m_sep );

		for ( std::vector<fs::CPath>::iterator itFilePath = rFullPaths.begin(); itFilePath != rFullPaths.end(); ++itFilePath )
			*itFilePath = m_poolDirPath / *itFilePath;

		return rFullPaths.size();
	}


	// file enumeration

	std::tstring JoinFiles( const fs::CPathEnumerator& enumerator )
	{
		return str::Join( enumerator.m_filePaths, ut::CTempFilePool::m_sep );
	}

	std::tstring JoinSubDirs( const fs::CPathEnumerator& enumerator )
	{
		return str::Join( enumerator.m_subDirPaths, ut::CTempFilePool::m_sep );
	}


	// enumeration with relative paths

	size_t EnumFilePaths( std::vector<fs::CPath>& rFilePaths, const fs::TDirPath& dirPath, SortType sortType /*= SortAscending*/,
						  const TCHAR* pWildSpec /*= _T("*")*/, fs::TEnumFlags flags /*= fs::EF_Recurse*/ )
	{
		fs::CRelativePathEnumerator found( dirPath, flags );
		fs::EnumFiles( &found, dirPath, pWildSpec );

		size_t addedCount = path::JoinUniquePaths( rFilePaths, found.m_filePaths );

		if ( sortType != NoSort )
			fs::SortPaths( rFilePaths, SortAscending == sortType );

		return addedCount;
	}

	size_t EnumSubDirPaths( std::vector<fs::TDirPath>& rSubDirPaths, const fs::TDirPath& dirPath, SortType sortType /*= SortAscending*/,
							fs::TEnumFlags flags /*= fs::EF_Recurse*/ )
	{
		fs::CRelativePathEnumerator found( dirPath, flags );
		fs::EnumFiles( &found, dirPath, _T("*") );

		size_t addedCount = path::JoinUniquePaths( rSubDirPaths, found.m_subDirPaths );

		if ( sortType != NoSort )
			fs::SortPaths( rSubDirPaths, SortAscending == sortType );

		return addedCount;
	}


	std::tstring EnumJoinFiles( const fs::TDirPath& dirPath, SortType sortType /*= SortAscending*/, const TCHAR* pWildSpec /*= _T("*")*/,
								fs::TEnumFlags flags /*= fs::EF_Recurse*/ )
	{
		std::vector<fs::CPath> filePaths;
		EnumFilePaths( filePaths, dirPath, sortType, pWildSpec, flags );

		return str::Join( filePaths, CTempFilePool::m_sep );
	}

	std::tstring EnumJoinSubDirs( const fs::TDirPath& dirPath, SortType sortType /*= SortAscending*/, fs::TEnumFlags flags /*= fs::EF_Recurse*/ )
	{
		std::vector<fs::TDirPath> subDirPaths;
		EnumSubDirPaths( subDirPaths, dirPath, sortType, flags );

		return str::Join( subDirPaths, CTempFilePool::m_sep );
	}


	fs::CPath FindFirstFile( const fs::TDirPath& dirPath, const TCHAR* pWildSpec /*= _T("*.*")*/, fs::TEnumFlags flags /*= fs::TEnumFlags()*/ )
	{
		return path::StripDirPrefix( fs::FindFirstFile( dirPath, pWildSpec, flags ), dirPath );
	}
}


#endif //USE_UT
