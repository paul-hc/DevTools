
#include "stdafx.h"
#include "RenameFilesTests.h"
#include "RenameItem.h"
#include "FileService.h"
#include "FileCommands.h"
#include "utl/ContainerUtilities.h"
#include "utl/PathGenerator.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


#ifdef _DEBUG		// no UT code in release builds


namespace ut
{
	bool FormatDestinations( ut::CPathPairPool* pPool, const std::tstring& format, UINT seqCount = 1 )
	{
		ASSERT_PTR( pPool );
		CPathGenerator gen( &pPool->m_pathPairs, format, seqCount );
		return gen.GeneratePairs();
	}
}


CRenameFilesTests::CRenameFilesTests( void )
{
	ut::CTestSuite::Instance().RegisterTestCase( this );		// self-registration
}

CRenameFilesTests& CRenameFilesTests::Instance( void )
{
	static CRenameFilesTests testCase;
	return testCase;
}

void CRenameFilesTests::TestRenameSimple( void )
{
	ut::CTempFilePairPool pool( _T("foo 1.txt|foo 3.txt|foo 5.txt") );

	ASSERT( ut::FormatDestinations( &pool, _T("foo #.txt") ) );

	std::vector< CRenameItem* > renameItems;
	ren::MakePairsToItems( renameItems, pool.m_pathPairs );

	CFileService svc;
	std::auto_ptr< CMacroCommand > pRenameMacroCmd( svc.MakeRenameCmds( renameItems ) );
	ASSERT_PTR( pRenameMacroCmd.get() );
	ASSERT( pRenameMacroCmd->Execute() );

	ASSERT_EQUAL( _T("foo 1.txt|foo 2.txt|foo 3.txt"), pool.JoinDest() );

	utl::ClearOwningContainer( renameItems );
}

void CRenameFilesTests::TestRenameCollisionExisting( void )
{
	// 2 types of collisions:
	//	- between SRC and DEST files
	//	- between existing files with duplicate format: "-[2]", "-[3]" -> will use intermediates "-[4]"

	const ut::CTempFilePairPool existingPool( _T("foo 2-[2].txt|foo 2-[3].txt|foo 3-[2].txt|foo 3-[3].txt|foo 4-[2].txt|foo 4-[3].txt|foo 5-[2].txt|foo 5-[3].txt|") );

	ut::CTempFilePairPool pool( _T("foo 1.txt|foo 2.txt|foo 3.txt|foo 4.txt|foo 5.txt") );

	ASSERT( ut::FormatDestinations( &pool, _T("foo #.txt"), 2 ) );

	std::vector< CRenameItem* > renameItems;
	ren::MakePairsToItems( renameItems, pool.m_pathPairs );

	CFileService svc;
	std::auto_ptr< CMacroCommand > pRenameMacroCmd( svc.MakeRenameCmds( renameItems ) );
	ASSERT_PTR( pRenameMacroCmd.get() );
	ASSERT( pRenameMacroCmd->Execute() );

	ASSERT_EQUAL( _T("foo 2.txt|foo 3.txt|foo 4.txt|foo 5.txt|foo 6.txt"), pool.JoinDest() );

	utl::ClearOwningContainer( renameItems );
}

void CRenameFilesTests::Run( void )
{
	__super::Run();

	TestRenameSimple();
	TestRenameCollisionExisting();
}


#endif //_DEBUG
