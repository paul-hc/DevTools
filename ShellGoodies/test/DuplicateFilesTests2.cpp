
#include "stdafx.h"

#ifdef USE_UT		// no UT code in release builds
#include "DuplicateFilesTests2.h"
#include "DuplicateFilesFinder.h"
#include "utl/ContainerUtilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace func
{
	struct ToOriginalItem
	{
		const CDuplicateFileItem* operator()( const CDuplicateFilesGroup* pDupGroup ) const
		{
			ASSERT_PTR( pDupGroup );
			return pDupGroup->GetOriginalItem();
		}
	};
}


namespace ut
{
	void SortDupGroups( std::vector< CDuplicateFilesGroup* >& rDupGroups )
	{
		typedef func::ValueAdapter< CPathItemBase::ToFilePath, func::ToOriginalItem > ToOriginalItemPath;
		typedef pred::CompareAdapter< pred::TCompareNameExt, ToOriginalItemPath > CompareOriginalNameExt;
		typedef pred::LessValue<CompareOriginalNameExt> TLess_OriginalNameExt;

		std::sort( rDupGroups.begin(), rDupGroups.end(), TLess_OriginalNameExt() );
	}

	std::tstring JoinRelativeDupPaths( const CDuplicateFilesGroup* pDupGroup, const fs::CPath& rootDir )
	{
		std::vector< fs::CPath > dupPaths;

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
	const fs::CPath& poolDirPath = pool.GetPoolDirPath();

	std::vector< fs::CPath > searchPathItems( 1, poolDirPath );
	std::vector< fs::CPath > ignorePathItems( 1, poolDirPath / _T("D1\\IGNORE") );

	std::vector< CDuplicateFilesGroup* > dupGroups;
	CDuplicateFilesFinder finder;

	finder.FindDuplicates( dupGroups, searchPathItems, ignorePathItems );

	ut::SortDupGroups( dupGroups );		// by original item filename

	ASSERT_EQUAL( 2, dupGroups.size() );
	ASSERT_EQUAL( _T("a.txt|D1\\a.txt|D1\\D2\\a.txt"), ut::JoinRelativeDupPaths( dupGroups[0], poolDirPath ) );
	ASSERT_EQUAL( _T("b.txt|D1\\D2\\b.txt"), ut::JoinRelativeDupPaths( dupGroups[1], poolDirPath ) );		// excluding D1\\IGNORE\\b.txt
}


void CDuplicateFilesTests::Run( void )
{
	__super::Run();

	TestDuplicateFiles();
}


#endif //USE_UT
