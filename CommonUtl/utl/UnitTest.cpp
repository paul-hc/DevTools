
#include "stdafx.h"
#include "UnitTest.h"
#include "ContainerUtilities.h"
#include "Path.h"
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
	const std::tstring& GetTestDataDirPath( void )
	{
		static std::tstring dirPath = str::ExpandEnvironmentStrings( _T("%UTL_TESTDATA_PATH%") );
		if ( !dirPath.empty() && !fs::IsValidDirectory( dirPath.c_str() ) )
		{
			TRACE( _T("\n * Cannot find the local test directory path: %s - define envirnoment variable UTL_TESTDATA_PATH\n"), dirPath.c_str() );
			dirPath.clear();
		}
		return dirPath;
	}

	std::tstring CombinePath( const std::tstring& parentDirPath, const TCHAR* pSubPath )
	{
		std::tstring newPath;
		if ( !parentDirPath.empty() && !str::IsEmpty( pSubPath ) )
			if ( fs::IsValidDirectory( parentDirPath.c_str() ) )
				newPath = path::Combine( parentDirPath.c_str(), pSubPath );

		return newPath;
	}


	std::tstring MakeNotEqualMessage( const std::tstring& expectedValue, const std::tstring& actualValue )
	{
		return str::Format( _T("* Equality Assertion Failed *\r\n\r\n  expected: '%s'\r\n  actual:   '%s'\r\n"), expectedValue.c_str(), actualValue.c_str() );
	}


	CTestSuite::~CTestSuite()
	{
	}

	CTestSuite& CTestSuite::Instance( void )
	{
		static CTestSuite singleton;
		return singleton;
	}

	void CTestSuite::RunUnitTests( void )
	{
		ASSERT( !IsEmpty() );
		for ( std::vector< ITestCase* >::const_iterator itTestCase = m_testCases.begin(); itTestCase != m_testCases.end(); ++itTestCase )
			( *itTestCase )->Run();
	}

	bool CTestSuite::RegisterTestCase( ut::ITestCase* pTestCase )
	{
		ASSERT_PTR( pTestCase );
		utl::PushUnique( m_testCases, pTestCase );
		return true;
	}

} //namespace ut


namespace ut
{
	// CTempFilePool implementation

	const TCHAR CTempFilePool::m_sep[] = _T("|");

	CTempFilePool::CTempFilePool( const TCHAR* pFlatPaths /*= NULL*/ )
		: m_tempDirPath( fs::MakeTempDirPath( _T("_UT") ) )
		, m_hasFileErrors( false )
	{
		fs::CreateDir( m_tempDirPath.c_str() );
		if ( !str::IsEmpty( pFlatPaths ) )
			SplitCreateFiles( pFlatPaths );
	}

	CTempFilePool::~CTempFilePool()
	{
		fs::DeleteDir( m_tempDirPath.c_str() );
	}

	bool CTempFilePool::SplitCreateFiles( const TCHAR* pFlatPaths /*= NULL*/ )
	{
		m_filePaths.clear();
		if ( !fs::IsValidDirectory( m_tempDirPath.c_str() ) )
			return false;

		m_hasFileErrors = false;

		std::vector< std::tstring > filePaths;
		str::Split( filePaths, pFlatPaths, m_sep );

		for ( std::vector< std::tstring >::iterator itSrcPath = filePaths.begin(); itSrcPath != filePaths.end(); ++itSrcPath )
		{
			*itSrcPath = path::Combine( m_tempDirPath.c_str(), itSrcPath->c_str() );		// convert to absolute path
			if ( !CreateFile( itSrcPath->c_str() ) )
			{
				m_hasFileErrors = true;
				return false;
			}
		}

		m_filePaths.reserve( filePaths.size() );
		for ( std::vector< std::tstring >::const_iterator itSrcPath = filePaths.begin(); itSrcPath != filePaths.end(); ++itSrcPath )
			m_filePaths.push_back( *itSrcPath );

		return true;
	}

	bool CTempFilePool::CreateFile( const TCHAR* pFilePath )
	{
		ASSERT_PTR( pFilePath );

		if ( fs::IsValidFile( pFilePath ) )
			return true;

		if ( !fs::CreateDirPath( fs::CPath( pFilePath ).GetDirPath().c_str() ) )
			return false;			// error creating sub dir

		std::ofstream output( str::ToUtf8( pFilePath ).c_str(), std::ios_base::out | std::ios_base::trunc );
		return fs::IsValidFile( pFilePath );
	}

	bool CTempFilePool::DeleteAllFiles( void )
	{
		return fs::DeleteAllFiles( m_tempDirPath.c_str() );
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

	std::tstring CPathPairPool::UnsplitDest( void )
	{
		std::vector< std::tstring > destFilenames; destFilenames.reserve( m_pathPairs.size() );
		for ( fs::PathPairMap::const_iterator it = m_pathPairs.begin(); it != m_pathPairs.end(); ++it )
			destFilenames.push_back( m_fullDestPaths ? it->second.Get() : fs::CPathParts( it->second.Get() ).GetNameExt() );

		return str::Unsplit( destFilenames, ut::CTempFilePool::m_sep );
	}

	void CPathPairPool::CopySrc( void )
	{
		for ( fs::PathPairMap::iterator it = m_pathPairs.begin(); it != m_pathPairs.end(); ++it )
			it->second = it->first;
	}


	// CTempFilePairPool implementation

	CTempFilePairPool::CTempFilePairPool( const TCHAR* pSourceFilenames )
		: CTempFilePool( pSourceFilenames )
	{
		if ( IsValidPool() )
			for ( std::vector< std::tstring >::const_iterator itSrcPath = GetFilePaths().begin(); itSrcPath != GetFilePaths().end(); ++itSrcPath )
				m_pathPairs[ *itSrcPath ] = fs::CPath();
	}

} //namespace ut


#endif //_DEBUG
