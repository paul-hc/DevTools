
#include "stdafx.h"
#include "TestCase.h"
#include "utl/FileSystem.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


#ifdef _DEBUG		// no UT code in release builds


CTestCase::CTestCase( void )
{
	ut::CTestSuite::Instance().RegisterTestCase( this );		// self-registration
}

CTestCase& CTestCase::Instance( void )
{
	static CTestCase testCase;
	return testCase;
}

void CTestCase::TestPullLossy( void )
{
	static const TCHAR* pSrcFiles = _T("\
A\\B\\fld.jpg|\
A\\B\\t1.flac|\
A\\B\\t1.mp3|\
A\\B\\t2.flac|\
A\\B\\t2.mp3|\
A\\B\\t3.flac|\
A\\B\\t3.mp3|\
A\\C\\fld.jpg|\
A\\C\\x1.flac|\
A\\C\\x1.mp3|\
A\\C\\x2.flac|\
A\\C\\x2.mp3\
");

	// test with physical files pool
	ut::CTempFilePairPool pool( pSrcFiles );
	const TCHAR* pTempDirPath = pool.GetTempDirPath().c_str();

	{
		fs::CEnumerator found( pTempDirPath );
		fs::EnumFiles( &found, pTempDirPath, _T("*.*"), Deep );
		ASSERT_EQUAL( pSrcFiles, ut::JoinFiles( found ) );
	}
}

void CTestCase::Run( void )
{
	__super::Run();

	TestPullLossy();
}


#endif //_DEBUG
