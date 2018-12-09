
#include "stdafx.h"
#include "TransferFuncTests.h"
#include "utl/FileSystem.h"
#include "utl/ProcessUtils.h"
#include "utl/StringUtilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


#ifdef _DEBUG		// no UT code in release builds


const TCHAR CTransferFuncTests::s_srcFiles[] =
	_T("SRC\\B\\fld.jpg|")
	_T("SRC\\B\\t1.flac|")
	_T("SRC\\B\\t1.mp3|")
	_T("SRC\\B\\t2.flac|")
	_T("SRC\\B\\t2.mp3|")
	_T("SRC\\B\\t3.flac|")
	_T("SRC\\B\\t3.mp3|")
	_T("SRC\\C\\fld.jpg|")
	_T("SRC\\C\\x1.flac|")
	_T("SRC\\C\\x1.mp4|")
	_T("SRC\\C\\x2.flac|")
	_T("SRC\\C\\x2.m4a");

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

void CTransferFuncTests::AddExtraParams( utl::CProcess& rXferProcess )
{
	rXferProcess.AddParam( _T("-y") );				// skip all user prompts

	if ( m_debugChildProcs )
		rXferProcess.AddParam( _T("-debug") );		// to debug command line parsing
}

void CTransferFuncTests::TestTransferCopy( void )
{
	ut::CTempFilePairPool pool( s_srcFiles );
	fs::CPath poolDirPath = pool.GetPoolDirPath();

	ASSERT_EQUAL( s_srcFiles, ut::EnumJoinFiles( poolDirPath ) );

	const fs::CPath srcDirPath = poolDirPath / fs::CPath( _T("SRC") );
	const fs::CPath targetDirPath = poolDirPath / fs::CPath( _T("DEST") );

	// build command line as: <exe_folder>\xFer.exe "<pool_dir>\SRC\*.mp3;*.m4?;*.mp4" "<pool_dir>\DEST" -transfer=copy

	utl::CProcess xferProcess( __targv[ 0 ] );
	xferProcess.AddParam( srcDirPath / fs::CPath( _T("*.mp3;*.m4?;*.mp4") ) );				// source_filter
	xferProcess.AddParam( targetDirPath );
	// note: "-transfer=copy" is an implicit arg; let's not pass it, to verify that it works implicitly

	AddExtraParams( xferProcess );

	ASSERT_EQUAL( 0, xferProcess.Execute() );		// no execution errors?

	ASSERT_EQUAL(
		_T("B\\t1.mp3|")
		_T("B\\t2.mp3|")
		_T("B\\t3.mp3|")
		_T("C\\x1.mp4|")
		_T("C\\x2.m4a"),
		ut::EnumJoinFiles( targetDirPath ) );

	// BACKUP test
	ut::CTempFilePairPool::ModifyTextFile( poolDirPath / fs::CPath( _T("SRC\\B\\t1.mp3") ) );
	ut::CTempFilePairPool::ModifyTextFile( poolDirPath / fs::CPath( _T("SRC\\C\\x2.m4a") ) );

	// redo the same copy transfer, but with backing up destination files this time
	xferProcess.AddParam( _T("/bk") );
	//xferProcess.AddParam( _T("-debug") );		// to debug command line parsing
	ASSERT_EQUAL( 0, xferProcess.Execute() );

return;
	ASSERT_EQUAL(
		_T("B\\t1.mp3|")
		_T("B\\t1-[2].mp3|")
		_T("B\\t2.mp3|")
		_T("B\\t3.mp3|")
		_T("C\\x1.mp4|")
		_T("C\\x2.m4a|")
		_T("C\\x2-[2].m4a"),
		ut::EnumJoinFiles( targetDirPath ) );

	ASSERT_EQUAL(
		_T("B\t1-[2].mp3|")
		_T("B\t1.mp3|")
		_T("B\t2-[2].mp3|")
		_T("B\t2.mp3|")
		_T("B\t3-[2].mp3|")
		_T("B\t3.mp3|")
		_T("C\x1-[2].mp4|")
		_T("C\x1.mp4|")
		_T("C\x2-[2].m4a|")
		_T("C\x2.m4a"),
		ut::EnumJoinFiles( targetDirPath ) );
}

void CTransferFuncTests::TestPullLossy( void )
{
	ut::CTempFilePairPool pool( s_srcFiles );
	fs::CPath poolDirPath = pool.GetPoolDirPath();

	const fs::CPath srcDirPath = poolDirPath / fs::CPath( _T("SRC") );
	const fs::CPath targetDirPath = poolDirPath / fs::CPath( _T("DEST") );

	{	// command: "<exe_folder>\xFer.exe <pool_dir>\SRC\*.mp3;*.m4?;*.mp4 <pool_dir>\DEST /transfer:move"
		utl::CProcess xferProcess( __targv[ 0 ] );
		xferProcess.AddParam( srcDirPath / fs::CPath( _T("*.mp3;*.m4?;*.mp4") ) );				// source_filter
		xferProcess.AddParam( targetDirPath );
		xferProcess.AddParam( _T("-transfer=move") );
		AddExtraParams( xferProcess );

		ASSERT_EQUAL( 0, xferProcess.Execute() );		// no execution errors?
	}

	{	// command: "<exe_folder>\xFer.exe <pool_dir>\SRC\f*.jp* <pool_dir>\DEST -ud"
		utl::CProcess xferProcess( __targv[ 0 ] );
		xferProcess.AddParam( srcDirPath / fs::CPath( _T("f*.jp*") ) );
		xferProcess.AddParam( targetDirPath );
		xferProcess.AddParam( _T("-ud") );
		AddExtraParams( xferProcess );

		ASSERT_EQUAL( 0, xferProcess.Execute() );		// no execution errors?
	}

	// verify SRC files
	ASSERT_EQUAL(
		_T("B\\fld.jpg|")
		_T("B\\t1.flac|")
		_T("B\\t2.flac|")
		_T("B\\t3.flac|")
		_T("C\\fld.jpg|")
		_T("C\\x1.flac|")
		_T("C\\x2.flac"),
		ut::EnumJoinFiles( srcDirPath ) );

	// verify DEST target files
	ASSERT_EQUAL(
		_T("B\\fld.jpg|")
		_T("B\\t1.mp3|")
		_T("B\\t2.mp3|")
		_T("B\\t3.mp3|")
		_T("C\\fld.jpg|")
		_T("C\\x1.mp4|")
		_T("C\\x2.m4a"),
		ut::EnumJoinFiles( targetDirPath ) );
}

void CTransferFuncTests::Run( void )
{
	__super::Run();

	TestTransferCopy();
	TestPullLossy();
}


#endif //_DEBUG
