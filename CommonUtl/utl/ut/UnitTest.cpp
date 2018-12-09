
#include "stdafx.h"
#include "ut/UnitTest.h"
#include "ContainerUtilities.h"
#include "FileSystem.h"
#include "Logger.h"
#include "Path.h"
#include "RuntimeException.h"
#include "StringUtilities.h"
#include <math.h>
#include <fstream>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


#ifdef _DEBUG		// no UT code in release builds


namespace numeric
{
	bool DoublesEqual( double left, double right )
	{
		return fabs( left - right ) < dEpsilon;
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
		static const fs::CPath dirPath = str::ExpandEnvironmentStrings( _T("%UTL_TESTDATA_PATH%") );
		if ( !dirPath.IsEmpty() && !fs::IsValidDirectory( dirPath.GetPtr() ) )
			throw CRuntimeException( str::Format( _T("Cannot find the local test directory path: %s\n\nTODO: define envirnoment variable UTL_TESTDATA_PATH."), dirPath.GetPtr() ) );

		return dirPath;
	}

	const fs::CPath& GetTempUt_DirPath( void ) throws_( CRuntimeException )
	{
		static fs::CPath tempUtDirPath;

		if ( tempUtDirPath.IsEmpty() )
		{
			tempUtDirPath = GetTestDataDirPath() / fs::CPath( _T("temp_ut") );
			fs::thr::CreateDirPath( tempUtDirPath.GetPtr() );
		}
		return tempUtDirPath;
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
		: m_poolDirPath( MakeTempUt_DirPath( fs::CPath( _T("_UT") ), true ) )
		, m_hasFileErrors( false )
	{
		fs::DeleteAllFiles( m_poolDirPath.GetPtr() );				// delete existing files from previously failed tests

		if ( !str::IsEmpty( pFlatPaths ) )
			SplitCreateFiles( pFlatPaths );
	}

	CTempFilePool::~CTempFilePool()
	{
		fs::DeleteDir( m_poolDirPath.GetPtr() );
	}

	bool CTempFilePool::DeleteAllFiles( void )
	{
		return fs::DeleteAllFiles( m_poolDirPath.GetPtr() );
	}

	bool CTempFilePool::SplitCreateFiles( const TCHAR* pFlatPaths /*= NULL*/ )
	{
		m_filePaths.clear();
		if ( !IsValidDir() )
			return false;

		m_hasFileErrors = false;

		std::vector< fs::CPath > filePaths;
		str::Split( filePaths, pFlatPaths, m_sep );

		for ( std::vector< fs::CPath >::iterator itSrcPath = filePaths.begin(); itSrcPath != filePaths.end(); ++itSrcPath )
		{
			*itSrcPath = m_poolDirPath / *itSrcPath;		// convert to absolute path
			if ( !CreateTextFile( *itSrcPath ) )
			{
				m_hasFileErrors = true;
				return false;
			}
		}

		m_filePaths.reserve( filePaths.size() );
		for ( std::vector< fs::CPath >::const_iterator itSrcPath = filePaths.begin(); itSrcPath != filePaths.end(); ++itSrcPath )
			m_filePaths.push_back( *itSrcPath );

		return true;
	}

	bool CTempFilePool::CreateTextFile( const fs::CPath& filePath )
	{
		if ( fs::IsValidFile( filePath.GetPtr() ) )
			return true;

		if ( !fs::CreateDirPath( filePath.GetParentPath().GetPtr() ) )
			return false;			// error creating sub dir

		std::ofstream output( filePath.GetUtf8().c_str(), std::ios_base::out | std::ios_base::trunc );
		output
			<< "Unit-test file: " << std::endl
			<< filePath.GetFilename() << std::endl;

		return filePath.FileExist();
	}

	bool CTempFilePool::ModifyTextFile( const fs::CPath& filePath )
	{
		if ( !filePath.FileExist() )
		{
			ASSERT( false );
			return false;
		}
		std::ofstream output( filePath.GetUtf8().c_str(), std::ios_base::out | std::ios_base::app );
		output << filePath.GetFilename() << std::endl;
		return filePath.FileExist();
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
			destFilenames.push_back( m_fullDestPaths ? itPair->second.Get() : fs::CPathParts( itPair->second.Get() ).GetNameExt() );

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


	// enumeration with relative paths

	size_t EnumFiles( std::vector< fs::CPath >& rFilePaths, const fs::CPath& dirPath, SortType sortType /*= SortAscending*/,
					  const TCHAR* pWildSpec /*= _T("*")*/, RecursionDepth depth /*= Deep*/ )
	{
		fs::CEnumerator found( dirPath.Get() );
		fs::EnumFiles( &found, dirPath.GetPtr(), pWildSpec, depth );

		rFilePaths.assign( found.m_filePaths.begin(), found.m_filePaths.end() );

		if ( sortType != NoSort )
			fs::SortPaths( rFilePaths, SortAscending == sortType );

		return rFilePaths.size();
	}

	size_t EnumSubDirs( std::vector< fs::CPath >& rSubDirPaths, const fs::CPath& dirPath, SortType sortType /*= SortAscending*/,
						RecursionDepth depth /*= Deep*/ )
	{
		fs::CEnumerator found( dirPath.Get() );
		fs::EnumFiles( &found, dirPath.GetPtr(), _T("*"), depth );

		rSubDirPaths.assign( found.m_subDirPaths.begin(), found.m_subDirPaths.end() );

		if ( sortType != NoSort )
			fs::SortPaths( rSubDirPaths, SortAscending == sortType );

		return rSubDirPaths.size();
	}


	std::tstring EnumJoinFiles( const fs::CPath& dirPath, SortType sortType /*= SortAscending*/, const TCHAR* pWildSpec /*= _T("*")*/,
								RecursionDepth depth /*= Deep*/ )
	{
		std::vector< fs::CPath > filePaths;
		EnumFiles( filePaths, dirPath, sortType, pWildSpec, depth );

		return str::Join( filePaths, CTempFilePool::m_sep );
	}

	std::tstring EnumJoinSubDirs( const fs::CPath& dirPath, SortType sortType /*= SortAscending*/, RecursionDepth depth /*= Deep*/ )
	{
		std::vector< fs::CPath > subDirPaths;
		EnumSubDirs( subDirPaths, dirPath, sortType, depth );

		return str::Join( subDirPaths, CTempFilePool::m_sep );
	}
}


#endif //_DEBUG
