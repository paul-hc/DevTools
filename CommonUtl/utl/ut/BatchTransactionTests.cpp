
#include "stdafx.h"
#include "ut/BatchTransactionTests.h"
#include "Path.h"
#include "StringUtilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


#ifdef _DEBUG		// no UT code in release builds


CBatchTransactionTests::CBatchTransactionTests( void )
{
	ut::CTestSuite::Instance().RegisterTestCase( this );		// self-registration
}

CBatchTransactionTests& CBatchTransactionTests::Instance( void )
{
	static CBatchTransactionTests testCase;
	return testCase;
}

void CBatchTransactionTests::Test__( void )
{
	ut::CTempFilePairPool pool( _T("a|a.doc|a.txt|d1\\b|d1\\b.doc|d1\\b.txt|d1\\d2\\c|d1/d2/c.doc|d1\\d2\\c.txt") );
	const fs::CPath& tempDirPath = pool.GetPoolDirPath();

	{
		fs::CEnumerator found( tempDirPath.Get() );
		fs::EnumFiles( &found, tempDirPath.GetPtr(), _T("*.*"), Shallow );
		ASSERT_EQUAL( _T("a|a.doc|a.txt"), ut::JoinFiles( found ) );
	}
}

void CBatchTransactionTests::Run( void )
{
	__super::Run();

	Test__();
}


#endif //_DEBUG
