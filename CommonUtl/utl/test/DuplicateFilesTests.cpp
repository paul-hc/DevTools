
#include "pch.h"

#ifdef USE_UT		// no UT code in release builds
#include "DuplicateFilesTests.h"
#include "DuplicateFilesEnumerator.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace ut
{
	std::tstring JoinRelativeDupPaths( const CDuplicateFilesGroup* pDupGroup, const fs::TDirPath& rootDir )
	{
		std::vector<fs::CPath> dupPaths;

		func::QueryItemsPaths( dupPaths, pDupGroup->GetItems() );
		path::StripDirPrefixes( dupPaths, rootDir.GetPtr() );

		return str::Join( dupPaths, ut::CTempFilePool::m_sep );
	}
}


CDuplicateFilesTests::CDuplicateFilesTests( void )
{
	ut::CTestSuite::Instance().RegisterTestCase( this );		// self-registration
}

CDuplicateFilesTests& CDuplicateFilesTests::Instance( void )
{
	static CDuplicateFilesTests s_testCase;
	return s_testCase;
}

void CDuplicateFilesTests::TestDuplicateFiles( void )
{
	ut::CTempFilePool pool( _T("a.txt|b.txt|file1.txt|D1\\a.txt|D1\\file2.txt|D1\\D2\\a.txt|D1\\D2\\b.txt|D1\\D2\\file3.txt|D1\\D2\\file4.txt|D1\\IGNORE\\b.txt") );
	const fs::TDirPath& poolDirPath = pool.GetPoolDirPath();

	CDuplicateFilesEnumerator enumer( fs::EF_Recurse );
	enumer.RefOptions().m_ignorePathMatches.AddPath( poolDirPath / _T("D1\\IGNORE") );

	enumer.SearchDuplicates( poolDirPath );

	const CDupsOutcome& outcome = enumer.GetOutcome();
	ASSERT_EQUAL( 2, outcome.m_foundSubDirCount );		// D1, D1/D2
	ASSERT_EQUAL( 9, outcome.m_foundFileCount );
	ASSERT_EQUAL( 1, outcome.m_ignoredCount );			// D1/IGNORE

	const std::vector<CDuplicateFilesGroup*>& dupGroups = enumer.m_dupGroupItems;

	ASSERT_EQUAL( 2, dupGroups.size() );
	ASSERT_EQUAL( _T("a.txt|D1\\a.txt|D1\\D2\\a.txt"), ut::JoinRelativeDupPaths( dupGroups[0], poolDirPath ) );
	ASSERT_EQUAL( _T("b.txt|D1\\D2\\b.txt"), ut::JoinRelativeDupPaths( dupGroups[1], poolDirPath ) );		// excluding D1\\IGNORE\\b.txt
}


void CDuplicateFilesTests::Run( void )
{
	RUN_TEST( TestDuplicateFiles );
}


#endif //USE_UT
