
#include "pch.h"

#ifdef USE_UT		// no UT code in release builds
#include "RenameFilesTests.h"
#include "RenameItem.h"
#include "FileService.h"
#include "TextAlgorithms.h"
#include "AppCommands.h"
#include "utl/Algorithms.h"
#include "utl/ContainerOwnership.h"
#include "utl/Logger.h"
#include "utl/PathGenerator.h"
#include "utl/RandomUtilities.h"
#include "utl/test/TempFilePairPool.h"

#define new DEBUG_NEW


namespace ut
{
	bool FormatDestinations( ut::CPathPairPool* pPool, const CPathFormatter& formatter, UINT seqCount = 1 )
	{
		ASSERT_PTR( pPool );
		CPathGenerator gen( &pPool->m_pathPairs, formatter, seqCount );
		return gen.GeneratePairs();
	}

	template< typename FuncType >
	void ForEachDestination( std::vector<CRenameItem*>& rRenameItems, const FuncType& func )
	{
		for ( std::vector<CRenameItem*>::const_iterator itItem = rRenameItems.begin(); itItem != rRenameItems.end(); ++itItem )
		{
			fs::CPathParts destParts;
			(*itItem)->SplitSafeDestPath( &destParts );

			func( destParts );
			(*itItem)->RefDestPath() = destParts.MakePath();
		}
	}


	class CScopedCmdLogger
	{
	public:
		CScopedCmdLogger( CLogger* pLogger ) : m_pOldLogger( cmd::CBaseSerialCmd::s_pLogger ) { cmd::CBaseSerialCmd::s_pLogger = pLogger; }
		~CScopedCmdLogger() { cmd::CBaseSerialCmd::s_pLogger = m_pOldLogger; }
	private:
		CLogger* m_pOldLogger;
	};
}


CRenameFilesTests::CRenameFilesTests( void )
{
	ut::CTestSuite::Instance().RegisterTestCase( this );		// self-registration
}

CRenameFilesTests& CRenameFilesTests::Instance( void )
{
	static CRenameFilesTests s_testCase;
	return s_testCase;
}

void CRenameFilesTests::TestRenameSimple( void )
{
	ut::GetTestLogger().LogLine( _T("CRenameFilesTests::TestRenameSimple"), false );

	ut::CTempFilePairPool pool( _T("foo 1.txt|foo 3.txt|foo 5.txt") );

	ASSERT( ut::FormatDestinations( &pool, CPathFormatter( _T("foo #.txt"), false ) ) );

	std::vector<CRenameItem*> renameItems;
	ren::MakePairsToItems( renameItems, pool.m_pathPairs );

	CFileService svc;
	std::auto_ptr<CMacroCommand> pRenameMacroCmd( svc.MakeRenameCmds( renameItems ) );
	ASSERT_PTR( pRenameMacroCmd.get() );
	ASSERT_EQUAL( 3, pRenameMacroCmd->GetSubCommands().size() );

	ASSERT( pRenameMacroCmd->Execute() );
	ASSERT_EQUAL( _T("foo 1.txt|foo 2.txt|foo 3.txt"), ut::EnumJoinFiles( pool.GetPoolDirPath() ) );

	utl::ClearOwningContainer( renameItems );
}

void CRenameFilesTests::TestRenameCollisionExisting( void )
{
	ut::GetTestLogger().LogLine( _T("CRenameFilesTests::TestRenameCollisionExisting"), false );

	// 2 types of collisions:
	//	- between SRC and DEST files
	//	- between existing files with duplicate format: "-[2]", "-[3]" -> will use intermediates "-[4]"

	const ut::CTempFilePairPool existingPool( _T("foo 2-[2].txt|foo 2-[3].txt|foo 3-[2].txt|foo 3-[3].txt|foo 4-[2].txt|foo 4-[3].txt|foo 5-[2].txt|foo 5-[3].txt|") );

	ut::CTempFilePairPool pool( _T("foo 1.txt|foo 2.txt|foo 3.txt|foo 4.txt|foo 5.txt") );

	ASSERT( ut::FormatDestinations( &pool, CPathFormatter( _T("foo #.txt"), false ), 2 ) );

	std::vector<CRenameItem*> renameItems;
	ren::MakePairsToItems( renameItems, pool.m_pathPairs );

	CFileService svc;
	std::auto_ptr<CMacroCommand> pRenameMacroCmd( svc.MakeRenameCmds( renameItems ) );
	ASSERT_PTR( pRenameMacroCmd.get() );
	ASSERT_EQUAL( 9, pRenameMacroCmd->GetSubCommands().size() );		// plenty of intermediate paths

	ASSERT( pRenameMacroCmd->Execute() );
	ASSERT_EQUAL( _T("foo 2.txt|foo 3.txt|foo 4.txt|foo 5.txt|foo 6.txt"), ut::EnumJoinFiles( pool.GetPoolDirPath(), SortAscending, _T("foo ?.*") ) );

	utl::ClearOwningContainer( renameItems );
}

void CRenameFilesTests::TestRenameChangeCase( void )
{
	ut::GetTestLogger().LogLine( _T("CRenameFilesTests::TestRenameChangeCase"), false );

	ut::CTempFilePairPool pool( _T("foo a.txt|foo b.txt|foo c.txt") );

	std::vector<CRenameItem*> renameItems;
	ren::MakePairsToItems( renameItems, pool.m_pathPairs );

	ut::ForEachDestination( renameItems, func::MakeCase( UpperCase ) );

	CFileService svc;
	std::auto_ptr<CMacroCommand> pRenameMacroCmd( svc.MakeRenameCmds( renameItems ) );
	ASSERT_PTR( pRenameMacroCmd.get() );
	ASSERT_EQUAL( 3, pRenameMacroCmd->GetSubCommands().size() );

	ASSERT( pRenameMacroCmd->Execute() );
	ASSERT_EQUAL( _T("FOO A.TXT|FOO B.TXT|FOO C.TXT"), ut::EnumJoinFiles( pool.GetPoolDirPath() ) );

	utl::ClearOwningContainer( renameItems );
}

void CRenameFilesTests::MakeTestFileItems( void )
{
	const fs::TDirPath dirPath( _T("C:\\download\\#\\temp_dev\\items") );

	if ( !fs::IsValidDirectory( dirPath.GetPtr() ) )
	{
		ASSERT( false );
		return;
	}

	enum { Count = 30 };
	std::vector<size_t> fileIndexes, textIndexes;

	size_t idx = 0;
	std::generate_n( std::back_inserter( fileIndexes ), Count, [&idx]( void ) { return idx++; } );
	idx = 0;
	std::generate_n( std::back_inserter( textIndexes ), Count, [&idx]( void ) { return idx++; } );
	utl::SetRandomSeed();
	std::random_shuffle( fileIndexes.begin(), fileIndexes.end() );
	std::random_shuffle( textIndexes.begin(), textIndexes.end() );

	for ( size_t i = 0; i != Count; ++i )
	{
		fs::TFilePath filePath = dirPath / str::Format( _T("file_%02d.txt"), fileIndexes[i] );
		std::tstring text( textIndexes[i], _T('*') );

		ut::SetFileText( filePath, text.c_str() );
		::Sleep( 1100 );
	}
}

void CRenameFilesTests::Run( void )
{
	ut::CScopedCmdLogger scopeTestLogger( &ut::GetTestLogger() );

	//RUN_TEST( MakeTestFileItems );
	RUN_TEST( TestRenameSimple );
	RUN_TEST( TestRenameCollisionExisting );
	RUN_TEST( TestRenameChangeCase );
}


#endif //USE_UT
