
#include "stdafx.h"

#ifdef USE_UT		// no UT code in release builds
#include "AlgorithmsTests.h"
#include "UtlConsoleTests.h"
#include "StringTests.h"
#include "StringRangeTests.h"
#include "NumericTests.h"
#include "EndiannessTests.h"
#include "LcsTests.h"
#include "RegistryTests.h"
#include "ResequenceTests.h"
#include "FmtUtilsTests.h"
#include "PathTests.h"
#include "PathGeneratorTests.h"
#include "FileSystemTests.h"
#include "TextFileIoTests.h"
#include "StructuredStorageTest.h"
#include "ThreadingTests.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace ut
{
	void RegisterUtlConsoleTests( void )
	{
		// register UTL tests
		CAlgorithmsTests::Instance();
		CStringTests::Instance();
		CStringRangeTests::Instance();
		CNumericTests::Instance();
		CEndiannessTests::Instance();
		CLcsTests::Instance();
		CRegistryTests::Instance();
		CResequenceTests::Instance();
		CFmtUtilsTests::Instance();

		CPathTests::Instance();
		CPathGeneratorTests::Instance();
		CFileSystemTests::Instance();
		CTextFileIoTests::Instance();
		CStructuredStorageTest::Instance();
		CThreadingTests::Instance();
	}
}


#endif //USE_UT
