
#include "stdafx.h"

#ifdef _DEBUG		// no UT code in release builds
#include "test/UnitTest.h"
#include "ContainerUtilities.h"
#include "TextFileIo.h"
#include "FileEnumerator.h"
#include "Logger.h"
#include "Path.h"
#include "RuntimeException.h"
#include "StringUtilities.h"
#include <math.h>
#include <hash_set>
#include <fstream>
#include <iomanip>

#define new DEBUG_NEW


namespace numeric
{
	bool DoublesEqual( double left, double right )
	{
		return fabs( left - right ) < dEpsilon;
	}
}


namespace ut
{
	bool SetFileText( const fs::CPath& filePath, const TCHAR* pText /*= NULL*/ )
	{
		if ( !fs::CreateDirPath( filePath.GetParentPath().GetPtr() ) )
			return false;			// error creating sub dir

		if ( NULL == pText )
			pText = filePath.GetFilenamePtr();		// use the filename as text content

		std::ofstream output( filePath.GetPtr(), std::ios::out | std::ios::trunc );
		output
			<< "Unit-test file: " << std::endl
			<< pText << std::endl;

		return filePath.FileExist();
	}

	bool ModifyFileText( const fs::CPath& filePath, const TCHAR* pText /*= NULL*/, bool retainModifyTime /*= false*/ )
	{
		if ( !filePath.FileExist() )
		{
			ASSERT( false );
			return false;
		}

		std::auto_ptr< fs::CScopedFileTime > pScopedFileTime( retainModifyTime ? new fs::CScopedFileTime( filePath ) : NULL );

		if ( NULL == pText )
			pText = filePath.GetFilenamePtr();		// use the filename as text content

		{
			std::ofstream output( filePath.GetPtr(), std::ios::out | std::ios::app );
			output << pText << std::endl;
		}

		return true;
	}


	void HexDump( std::ostream& os, const fs::CPath& textPath, size_t rowByteCount /*= DefaultRowByteCount*/ ) throws_( CRuntimeException )
	{	// dump contents of filename to stdout in hex
		std::fstream is( textPath.GetPtr(), std::ios::in | std::ios::binary );
		if ( !is.is_open() )
			io::ThrowOpenForReading( textPath );

		static const char s_unprintableCh = '.';
		static const char s_blankCh = ' ';
		static const char s_columnSep[] = "  ";

		std::vector< char > inputRowBuff( rowByteCount );
		std::string charRow;

		for ( size_t i; !is.eof(); )
		{
			inputRowBuff.assign( rowByteCount, '\0' );

			is.read( &inputRowBuff[ 0 ], rowByteCount );
			inputRowBuff.resize( is.gcount() );		// cut to the actual read chars

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
		static CLogger testLogger( _T("%s_tests") );
		return testLogger;
	}


	std::tstring MakeNotEqualMessage( const std::tstring& expectedValue, const std::tstring& actualValue )
	{
		return str::Format( _T("* Equality Assertion Failed *\r\n\r\n  expect:\t'%s'\r\n  actual:\t'%s'\r\n"), expectedValue.c_str(), actualValue.c_str() );
	}

	const fs::CPath& GetTestDataDirPath( void ) throws_( CRuntimeException )
	{
		static const fs::CPath s_dirPath = str::ExpandEnvironmentStrings( _T("%UTL_TESTDATA_PATH%") );
		if ( !s_dirPath.IsEmpty() && !fs::IsValidDirectory( s_dirPath.GetPtr() ) )
			throw CRuntimeException( str::Format( _T("Cannot find the local test directory path: %s\n\nTODO: define envirnoment variable UTL_TESTDATA_PATH"), s_dirPath.GetPtr() ) );

		return s_dirPath;
	}

	const fs::CPath& GetImageSourceDirPath( void )
	{
		static fs::CPath s_imagesDirPath = str::ExpandEnvironmentStrings( _T("%UTL_THUMB_SRC_IMAGE_PATH%") );
		if ( !s_imagesDirPath.IsEmpty() && !fs::IsValidDirectory( s_imagesDirPath.GetPtr() ) )
		{
			TRACE( _T("\n # Cannot find unit test images dir path: %s #\nNo environment variable UTL_THUMB_SRC_IMAGE_PATH"), s_imagesDirPath.GetPtr() );
			s_imagesDirPath.Clear();
		}
		return s_imagesDirPath;
	}

	const fs::CPath& GetDestImagesDirPath( void )
	{
		static fs::CPath s_dirPath = GetTestDataDirPath() / fs::CPath( _T("images") );
		if ( !s_dirPath.IsEmpty() && !fs::IsValidDirectory( s_dirPath.GetPtr() ) )
		{
			TRACE( _T("\n * Cannot find the local test images dir path: %s\n"), s_dirPath.GetPtr() );
			s_dirPath.Clear();
		}
		return s_dirPath;
	}

	const fs::CPath& GetStdImageDirPath( void )
	{
		static fs::CPath s_stdImagesDirPath = GetTestDataDirPath() / fs::CPath( _T("std_test_images") );
		if ( !s_stdImagesDirPath.IsEmpty() && !fs::IsValidDirectory( s_stdImagesDirPath.GetPtr() ) )
		{
			TRACE( _T("\n # Cannot find unit test standard images dir path: %s #\nTODO: create directory %UTL_STD_SRC_IMAGE_PATH%\\std_test_images"), s_stdImagesDirPath.GetPtr() );
			s_stdImagesDirPath.Clear();
		}
		return s_stdImagesDirPath;
	}

	const fs::CPath& GetStdTestFilesDirPath( void )
	{
		static fs::CPath s_stdImagesDirPath = GetTestDataDirPath() / fs::CPath( _T("std_test_files") );
		if ( !s_stdImagesDirPath.IsEmpty() && !fs::IsValidDirectory( s_stdImagesDirPath.GetPtr() ) )
		{
			TRACE( _T("\n # Cannot find unit test standard test files dir path: %s #\nTODO: create directory %UTL_STD_SRC_IMAGE_PATH%\\std_test_files"), s_stdImagesDirPath.GetPtr() );
			s_stdImagesDirPath.Clear();
		}
		return s_stdImagesDirPath;
	}


	const fs::CPath& GetTempUt_DirPath( void ) throws_( CRuntimeException )
	{
		static fs::CPath s_tempUtDirPath;

		if ( s_tempUtDirPath.IsEmpty() )
		{
			s_tempUtDirPath = GetTestDataDirPath() / fs::CPath( _T("temp_ut") );
			fs::thr::CreateDirPath( s_tempUtDirPath.GetPtr() );
		}
		return s_tempUtDirPath;
	}

	fs::CPath MakeTempUt_DirPath( const fs::CPath& subPath, bool createDir ) throws_( CRuntimeException )
	{
		fs::CPath fullPath = GetTempUt_DirPath() / subPath;
		if ( createDir )
			fs::thr::CreateDirPath( fullPath.GetPtr() );
		return fullPath;
	}
}


namespace ut
{
	// CTempFilePool implementation

	const TCHAR CTempFilePool::m_sep[] = _T("|");

	CTempFilePool::CTempFilePool( const TCHAR* pFlatPaths /*= NULL*/ )
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

	fs::CPath CTempFilePool::MakePoolDirPath( bool createDir /*= false*/ )
	{
		return MakeTempUt_DirPath( fs::CPath( _T("_UT") ), createDir );
	}

	bool CTempFilePool::DeleteAllFiles( void )
	{
		return fs::DeleteAllFiles( m_poolDirPath.GetPtr() );
	}

	bool CTempFilePool::CreateFiles( const TCHAR* pFlatPaths /*= NULL*/ )
	{
		if ( !IsValidDir() )
			return false;

		m_hasFileErrors = false;

		std::vector< fs::CPath > filePaths;
		str::Split( filePaths, pFlatPaths, m_sep );

		for ( std::vector< fs::CPath >::iterator itSrcPath = filePaths.begin(); itSrcPath != filePaths.end(); ++itSrcPath )
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

	size_t CTempFilePool::SplitQualifyPaths( std::vector< fs::CPath >& rFullPaths, const TCHAR relFilePaths[] ) const
	{
		str::Split( rFullPaths, relFilePaths, m_sep );

		for ( std::vector< fs::CPath >::iterator itFilePath = rFullPaths.begin(); itFilePath != rFullPaths.end(); ++itFilePath )
			*itFilePath = m_poolDirPath / *itFilePath;

		return rFullPaths.size();
	}


	// CPathPairPool implementation

	CPathPairPool::CPathPairPool( const TCHAR* pSourceFilenames, bool fullDestPaths /*= false*/ )
		: m_fullDestPaths( fullDestPaths )
	{
		std::vector< std::tstring > srcFilenames;
		str::Split( srcFilenames, pSourceFilenames, ut::CTempFilePool::m_sep );
		for ( std::vector< std::tstring >::const_iterator itSrc = srcFilenames.begin(); itSrc != srcFilenames.end(); ++itSrc )
			m_pathPairs[ *itSrc ] = fs::CPath();

		ENSURE( m_pathPairs.size() == srcFilenames.size() );
	}

	std::tstring CPathPairPool::JoinDest( void )
	{
		std::vector< std::tstring > destFilenames; destFilenames.reserve( m_pathPairs.size() );
		for ( fs::TPathPairMap::const_iterator itPair = m_pathPairs.begin(); itPair != m_pathPairs.end(); ++itPair )
			destFilenames.push_back( m_fullDestPaths ? itPair->second.Get() : fs::CPathParts( itPair->second.Get() ).GetFilename() );

		return str::Join( destFilenames, ut::CTempFilePool::m_sep );
	}

	void CPathPairPool::CopySrc( void )
	{
		for ( fs::TPathPairMap::iterator itPair = m_pathPairs.begin(); itPair != m_pathPairs.end(); ++itPair )
			itPair->second = itPair->first;
	}


	// CTempFilePairPool implementation

	CTempFilePairPool::CTempFilePairPool( const TCHAR* pSourceFilenames )
		: CTempFilePool( pSourceFilenames )
	{
		if ( IsValidPool() )
			for ( std::vector< fs::CPath >::const_iterator itSrcPath = GetFilePaths().begin(); itSrcPath != GetFilePaths().end(); ++itSrcPath )
				m_pathPairs[ *itSrcPath ] = fs::CPath();
	}


	// file enumeration

	std::tstring JoinFiles( const fs::CEnumerator& enumerator )
	{
		return str::Join( enumerator.m_filePaths, ut::CTempFilePool::m_sep );
	}

	std::tstring JoinSubDirs( const fs::CEnumerator& enumerator )
	{
		return str::Join( enumerator.m_subDirPaths, ut::CTempFilePool::m_sep );
	}


	// enumeration with relative paths

	size_t EnumFilePaths( std::vector< fs::CPath >& rFilePaths, const fs::CPath& dirPath, SortType sortType /*= SortAscending*/,
						  const TCHAR* pWildSpec /*= _T("*")*/, RecursionDepth depth /*= Deep*/ )
	{
		fs::CRelativeEnumerator found( dirPath );
		fs::EnumFiles( &found, dirPath, pWildSpec, depth );

		size_t addedCount = fs::JoinUniquePaths( rFilePaths, found.m_filePaths );

		if ( sortType != NoSort )
			fs::SortPaths( rFilePaths, SortAscending == sortType );

		return addedCount;
	}

	size_t EnumSubDirPaths( std::vector< fs::CPath >& rSubDirPaths, const fs::CPath& dirPath, SortType sortType /*= SortAscending*/,
							RecursionDepth depth /*= Deep*/ )
	{
		fs::CRelativeEnumerator found( dirPath );
		fs::EnumFiles( &found, dirPath, _T("*"), depth );

		size_t addedCount = fs::JoinUniquePaths( rSubDirPaths, found.m_subDirPaths );

		if ( sortType != NoSort )
			fs::SortPaths( rSubDirPaths, SortAscending == sortType );

		return addedCount;
	}


	std::tstring EnumJoinFiles( const fs::CPath& dirPath, SortType sortType /*= SortAscending*/, const TCHAR* pWildSpec /*= _T("*")*/,
								RecursionDepth depth /*= Deep*/ )
	{
		std::vector< fs::CPath > filePaths;
		EnumFilePaths( filePaths, dirPath, sortType, pWildSpec, depth );

		return str::Join( filePaths, CTempFilePool::m_sep );
	}

	std::tstring EnumJoinSubDirs( const fs::CPath& dirPath, SortType sortType /*= SortAscending*/, RecursionDepth depth /*= Deep*/ )
	{
		std::vector< fs::CPath > subDirPaths;
		EnumSubDirPaths( subDirPaths, dirPath, sortType, depth );

		return str::Join( subDirPaths, CTempFilePool::m_sep );
	}


	fs::CPath FindFirstFile( const fs::CPath& dirPath, const TCHAR* pWildSpec /*= _T("*.*")*/, RecursionDepth depth /*= Shallow*/ )
	{
		return fs::StripDirPrefix( fs::FindFirstFile( dirPath, pWildSpec, depth ), dirPath );
	}
}


#endif //_DEBUG
