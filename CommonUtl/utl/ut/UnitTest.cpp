
#include "stdafx.h"
#include "ut/UnitTest.h"
#include "ContainerUtilities.h"
#include "Path.h"
#include "RuntimeException.h"
#include "StringUtilities.h"
#include <math.h>
#include <fstream>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


#ifdef _DEBUG		// no UT code in release builds


namespace ut
{
	const std::string& GetTestCaseName( const ITestCase* pTestCase )
	{
		ASSERT_PTR( pTestCase );

		static std::string testName;
		testName = typeid( *pTestCase ).name();		// name of the derived concrete class (dereferenced pointer)
		str::StripPrefix( testName, "class " );
		return testName;
	}

	void CConsoleTestCase::Run( void )
	{
		TRACE( "-- %s console test case --\n", GetTestCaseName( this ).c_str() );
	}

	void CGraphicTestCase::Run( void )
	{
		TRACE( "-- %s graphic test case --\n", GetTestCaseName( this ).c_str() );
	}
}


namespace numeric
{
	bool DoublesEqual( double left, double right )
	{
		return fabs( left - right ) < dEpsilon;
	}
}


namespace ut
{
	std::tstring MakeNotEqualMessage( const std::tstring& expectedValue, const std::tstring& actualValue )
	{
		return str::Format( _T("* Equality Assertion Failed *\r\n\r\n  expect:\t'%s'\r\n  actual:\t'%s'\r\n"), expectedValue.c_str(), actualValue.c_str() );
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
			fs::EnsureDirPath( tempUtDirPath.GetPtr() );
		}
		return tempUtDirPath;
	}

	fs::CPath MakeTempUt_DirPath( const fs::CPath& subPath, bool createDir ) throws_( CRuntimeException )
	{
		fs::CPath fullPath = GetTempUt_DirPath() / subPath;
		if ( createDir )
			fs::EnsureDirPath( fullPath.GetPtr() );
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
		if ( !str::IsEmpty( pFlatPaths ) )
			SplitCreateFiles( pFlatPaths );
	}

	CTempFilePool::~CTempFilePool()
	{
		fs::DeleteDir( m_poolDirPath.GetPtr() );
	}

	bool CTempFilePool::SplitCreateFiles( const TCHAR* pFlatPaths /*= NULL*/ )
	{
		m_filePaths.clear();
		if ( !IsValidDir() )
			return false;

		m_hasFileErrors = false;

		std::vector< std::tstring > filePaths;
		str::Split( filePaths, pFlatPaths, m_sep );

		for ( std::vector< std::tstring >::iterator itSrcPath = filePaths.begin(); itSrcPath != filePaths.end(); ++itSrcPath )
		{
			*itSrcPath = path::Combine( m_poolDirPath.GetPtr(), itSrcPath->c_str() );		// convert to absolute path
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

		if ( !fs::CreateDirPath( fs::CPath( pFilePath ).GetParentPath().GetPtr() ) )
			return false;			// error creating sub dir

		std::ofstream output( str::ToUtf8( pFilePath ).c_str(), std::ios_base::out | std::ios_base::trunc );
		return fs::IsValidFile( pFilePath );
	}

	bool CTempFilePool::DeleteAllFiles( void )
	{
		return fs::DeleteAllFiles( m_poolDirPath.GetPtr() );
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
}


#endif //_DEBUG
