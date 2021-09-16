
#include "stdafx.h"
#include "TransferFuncTests.h"
#include "utl/FileSystem.h"
#include "utl/ProcessCmd.h"
#include "utl/StringUtilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


#ifdef _DEBUG		// no UT code in release builds


const TCHAR CTransferFuncTests::s_srcFiles[] =
	_T("SRC\\B\\b1.flac|")
	_T("SRC\\B\\b1.mp3|")
	_T("SRC\\B\\b2.flac|")
	_T("SRC\\B\\b2.mp3|")
	_T("SRC\\B\\b3.flac|")
	_T("SRC\\B\\b3.mp3|")
	_T("SRC\\B\\fld.jpg|")
	_T("SRC\\C\\c1.flac|")
	_T("SRC\\C\\c1.mp4|")
	_T("SRC\\C\\c2.flac|")
	_T("SRC\\C\\c2.m4a|")
	_T("SRC\\C\\fld.jpg");

const fs::CPath CTransferFuncTests::s_lossyFilter( _T("*.mp3;*.m4?;*.mp4") );

CTransferFuncTests::CTransferFuncTests( void )
	: m_debugChildProcs( false )
{
	ut::CTestSuite::Instance().RegisterTestCase( this );		// self-registration
}

CTransferFuncTests& CTransferFuncTests::Instance( void )
{
	static CTransferFuncTests s_testCase;
	return s_testCase;
}

int CTransferFuncTests::ExecuteProcess( utl::CProcessCmd& rProcess )
{
	rProcess.AddParam( _T("-y") );				// skip all user prompts
	if ( m_debugChildProcs )
		rProcess.AddParam( _T("-debug") );		// to debug command line parsing

	return rProcess.Execute();
}

void CTransferFuncTests::TestCopy( void )
{
	ut::CTempFilePairPool pool( s_srcFiles );
	fs::CPath poolDirPath = pool.GetPoolDirPath();

	ASSERT_EQUAL( s_srcFiles, ut::EnumJoinFiles( poolDirPath ) );

	const fs::CPath srcDirPath = poolDirPath / fs::CPath( _T("SRC") );
	const fs::CPath targetDirPath = poolDirPath / fs::CPath( _T("TARGET") );

	// build command line as: <exe_folder>\xFer.exe "<pool_dir>\SRC\*.mp3;*.m4?;*.mp4" "<pool_dir>\TARGET"
	utl::CProcessCmd copyLossy( __targv[ 0 ] );
	copyLossy.AddParam( srcDirPath / s_lossyFilter );	// source_filter
	copyLossy.AddParam( targetDirPath );
		// note: "-transfer=copy" is an implicit arg; let's not pass it, to ensure that it works implicitly

	ASSERT_EQUAL( 0, ExecuteProcess( copyLossy ) );		// no execution errors?

	ASSERT_EQUAL(
		_T("B\\b1.flac|")
		_T("B\\b1.mp3|")
		_T("B\\b2.flac|")
		_T("B\\b2.mp3|")
		_T("B\\b3.flac|")
		_T("B\\b3.mp3|")
		_T("B\\fld.jpg|")
		_T("C\\c1.flac|")
		_T("C\\c1.mp4|")
		_T("C\\c2.flac|")
		_T("C\\c2.m4a|")
		_T("C\\fld.jpg")
		, ut::EnumJoinFiles( srcDirPath ) );

	ASSERT_EQUAL(
		_T("B\\b1.mp3|")
		_T("B\\b2.mp3|")
		_T("B\\b3.mp3|")
		_T("C\\c1.mp4|")
		_T("C\\c2.m4a")
		, ut::EnumJoinFiles( targetDirPath ) );
}

void CTransferFuncTests::TestMove( void )
{
	ut::CTempFilePairPool pool( s_srcFiles );
	fs::CPath poolDirPath = pool.GetPoolDirPath();

	ASSERT_EQUAL( s_srcFiles, ut::EnumJoinFiles( poolDirPath ) );

	const fs::CPath srcDirPath = poolDirPath / fs::CPath( _T("SRC") );
	const fs::CPath targetDirPath = poolDirPath / fs::CPath( _T("TARGET") );

	// build command line as: <exe_folder>\xFer.exe "<pool_dir>\SRC\*.mp3;*.m4?;*.mp4" "<pool_dir>\TARGET"
	utl::CProcessCmd moveLossy( __targv[ 0 ] );
	moveLossy.AddParam( srcDirPath / s_lossyFilter );	// source_filter
	moveLossy.AddParam( targetDirPath );
	moveLossy.AddParam( _T("-transfer=move") );

	ASSERT_EQUAL( 0, ExecuteProcess( moveLossy ) );

	ASSERT_EQUAL(
		_T("B\\b1.flac|")
		_T("B\\b2.flac|")
		_T("B\\b3.flac|")
		_T("B\\fld.jpg|")
		_T("C\\c1.flac|")
		_T("C\\c2.flac|")
		_T("C\\fld.jpg")
		, ut::EnumJoinFiles( srcDirPath ) );

	ASSERT_EQUAL(
		_T("B\\b1.mp3|")
		_T("B\\b2.mp3|")
		_T("B\\b3.mp3|")
		_T("C\\c1.mp4|")
		_T("C\\c2.m4a")
		, ut::EnumJoinFiles( targetDirPath ) );
}

void CTransferFuncTests::TestBackup( void )
{
	ut::CTempFilePairPool pool( s_srcFiles );
	fs::CPath poolDirPath = pool.GetPoolDirPath();

	ASSERT_EQUAL( s_srcFiles, ut::EnumJoinFiles( poolDirPath ) );

	const fs::CPath srcDirPath = poolDirPath / fs::CPath( _T("SRC") );
	const fs::CPath targetDirPath = poolDirPath / fs::CPath( _T("TARGET") );

	{	// build command line as: <exe_folder>\xFer.exe "<pool_dir>\SRC\*.mp3;*.m4?;*.mp4" "<pool_dir>\TARGET"
		utl::CProcessCmd copyLossy( __targv[ 0 ] );
		copyLossy.AddParam( srcDirPath / s_lossyFilter );	// source_filter
		copyLossy.AddParam( targetDirPath );
		// note: "-transfer=copy" is an implicit arg; let's not pass it, to ensure that it works implicitly

		ASSERT_EQUAL( 0, ExecuteProcess( copyLossy ) );

		ASSERT_EQUAL(
			_T("B\\b1.mp3|")
			_T("B\\b2.mp3|")
			_T("B\\b3.mp3|")
			_T("C\\c1.mp4|")
			_T("C\\c2.m4a")
			, ut::EnumJoinFiles( targetDirPath ) );
	}

	// *** copy with BACKUP

	const fs::CPath b1_mp3Path = poolDirPath / fs::CPath( _T("SRC\\B\\b1.mp3") );

	// backup with newer SRC timestamp
	fs::thr::TouchFileBy( b1_mp3Path, 30 );					// change SRC timestamp by adding 30 seconds

	UT_REPEAT_BLOCK( 2 )				// repeat once to ensure a subsequent copy doesn't backup again
	{	// build command line as: <exe_folder>\xFer.exe "<pool_dir>\SRC\*.mp3;*.m4?;*.mp4" "<pool_dir>\TARGET"
		utl::CProcessCmd copyBackup( __targv[ 0 ] );
		copyBackup.AddParam( srcDirPath / s_lossyFilter );	// source_filter
		copyBackup.AddParam( targetDirPath );
		copyBackup.AddParam( _T("/ch") );					// copy only source files with timestamp newer than that of the existing destination
		copyBackup.AddParam( _T("/bk") );					// backing up destination files on transfer
		//copyBackup.AddParam( _T("-debug") );				// to debug command line parsing

		ASSERT_EQUAL( 0, ExecuteProcess( copyBackup ) );

		ASSERT_EQUAL(
			_T("B\\b1.mp3|")
			_T("B\\b1-[2].mp3|")		// backed-up due to newer timestamp
			_T("B\\b2.mp3|")
			_T("B\\b3.mp3|")
			_T("C\\c1.mp4|")
			_T("C\\c2.m4a")
			, ut::EnumJoinFiles( targetDirPath ) );
	}

	// backup with changed SRC content (same timestamp)
	ut::ModifyFileText( b1_mp3Path, NULL, true );			// add another line but retain timestamp, so that will check for the file size change

	{	// build command line as: <exe_folder>\xFer.exe "<pool_dir>\SRC\*.mp3;*.m4?;*.mp4" "<pool_dir>\TARGET"
		utl::CProcessCmd copyBackup( __targv[ 0 ] );
		copyBackup.AddParam( srcDirPath / s_lossyFilter );	// source_filter
		copyBackup.AddParam( targetDirPath );
		copyBackup.AddParam( _T("/ch=size") );				// copy only changed sources
		copyBackup.AddParam( _T("/bk") );					// backing up destination files on transfer
		//copyBackup.AddParam( _T("-debug") );				// to debug command line parsing

		ASSERT_EQUAL( 0, ExecuteProcess( copyBackup ) );

		// same as before since "b1.mp3" was already backed up to "b1-[2].mp3"
		ASSERT_EQUAL(
			_T("B\\b1.mp3|")
			_T("B\\b1-[2].mp3|")		// backed-up previously
			_T("B\\b2.mp3|")
			_T("B\\b3.mp3|")
			_T("C\\c1.mp4|")
			_T("C\\c2.m4a")
			, ut::EnumJoinFiles( targetDirPath ) );

		// backup with changed SRC content (same timestamp) -> second time it should create a new backup 
		ut::ModifyFileText( b1_mp3Path, NULL, true );								// one more content change to trigger a new backup "b1.mp3" -> "b1-[3].mp3"
		ut::ModifyFileText( poolDirPath / fs::CPath( _T("SRC\\C\\c2.m4a") ) );		// trigger a backup "c2.m4a" -> "c2-[2].m4a"

		ASSERT_EQUAL( 0, ExecuteProcess( copyBackup ) );

		ASSERT_EQUAL(
			_T("B\\b1.mp3|")
			_T("B\\b1-[2].mp3|")		// backed-up previously
			_T("B\\b1-[3].mp3|")		// new backup "b1.mp3" -> "b1-[3].mp3" (due to different file size)
			_T("B\\b2.mp3|")
			_T("B\\b3.mp3|")
			_T("C\\c1.mp4|")
			_T("C\\c2.m4a|")
			_T("C\\c2-[2].m4a")
			, ut::EnumJoinFiles( targetDirPath ) );
	}

	// backup all to a different directory
	{	// build command line as: <exe_folder>\xFer.exe "<pool_dir>\SRC\*.mp3;*.m4?;*.mp4" "<pool_dir>\TARGET"
		const fs::CPath backupDirPath = poolDirPath / fs::CPath( _T("Backup/vault") );

		utl::CProcessCmd transferBackup( __targv[ 0 ] );
		transferBackup.AddParam( srcDirPath / s_lossyFilter );	// source_filter
		transferBackup.AddParam( targetDirPath );
		transferBackup.AddParam( str::Format( _T("/bk=%s"), backupDirPath.GetPtr() ) );					// redo the same copy transfer, but with backing up destination files this time

		UT_REPEAT_BLOCK( 2 )				// repeat once to ensure a subsequent copy doesn't backup again
		{
			ASSERT_EQUAL( 0, ExecuteProcess( transferBackup ) );

			// all new backups to the new "Backup/vault" directory
			ASSERT_EQUAL(
				_T("b1.mp3|")
				_T("b2.mp3|")
				_T("b3.mp3|")
				_T("c1.mp4|")
				_T("c2.m4a")
				, ut::EnumJoinFiles( backupDirPath ) );			// files in "Backup/vault", with no deep paths
		}
	}
}

void CTransferFuncTests::TestPullLossy( void )
{
	ut::CTempFilePairPool pool( s_srcFiles );
	fs::CPath poolDirPath = pool.GetPoolDirPath();

	const fs::CPath srcDirPath = poolDirPath / fs::CPath( _T("SRC") );
	const fs::CPath targetDirPath = poolDirPath / fs::CPath( _T("TARGET") );

	{	// command: "<exe_folder>\xFer.exe <pool_dir>\SRC\*.mp3;*.m4?;*.mp4 <pool_dir>\TARGET /transfer:move"
		utl::CProcessCmd moveLossy( __targv[ 0 ] );
		moveLossy.AddParam( srcDirPath / s_lossyFilter );	// source_filter
		moveLossy.AddParam( targetDirPath );
		moveLossy.AddParam( _T("-transfer=move") );

		ASSERT_EQUAL( 0, ExecuteProcess( moveLossy ) );
	}

	{	// command: "<exe_folder>\xFer.exe <pool_dir>\SRC\f*.jp* <pool_dir>\TARGET -ud"
		utl::CProcessCmd copyImages( __targv[ 0 ] );
		copyImages.AddParam( srcDirPath / fs::CPath( _T("f*.jp*") ) );
		copyImages.AddParam( targetDirPath );
		copyImages.AddParam( _T("-ud") );

		ASSERT_EQUAL( 0, ExecuteProcess( copyImages ) );	// no execution errors?
	}

	// verify SRC files
	ASSERT_EQUAL(
		_T("B\\b1.flac|")
		_T("B\\b2.flac|")
		_T("B\\b3.flac|")
		_T("B\\fld.jpg|")
		_T("C\\c1.flac|")
		_T("C\\c2.flac|")
		_T("C\\fld.jpg")
		, ut::EnumJoinFiles( srcDirPath ) );

	// verify TARGET files
	ASSERT_EQUAL(
		_T("B\\b1.mp3|")
		_T("B\\b2.mp3|")
		_T("B\\b3.mp3|")
		_T("B\\fld.jpg|")
		_T("C\\c1.mp4|")
		_T("C\\c2.m4a|")
		_T("C\\fld.jpg")
		, ut::EnumJoinFiles( targetDirPath ) );
}

void CTransferFuncTests::Run( void )
{
	__super::Run();

	TestCopy();
	TestMove();
	TestBackup();
	TestPullLossy();
}


#endif //_DEBUG
