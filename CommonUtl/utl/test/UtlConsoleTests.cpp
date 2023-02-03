
#include "stdafx.h"

#ifdef USE_UT		// no UT code in release builds
#include "AlgorithmsTests.h"
#include "UtlConsoleTests.h"
#include "StringTests.h"
#include "StringRangeTests.h"
#include "NumericTests.h"
#include "EndiannessTests.h"
#include "EnvironmentTests.h"
#include "LcsTests.h"
#include "RegistryTests.h"
#include "ResequenceTests.h"
#include "FmtUtilsTests.h"
#include "PathTests.h"
#include "PathGeneratorTests.h"
#include "FileSystemTests.h"
#include "TextFileIoTests.h"
#include "StructuredStorageTest.h"
#include "DuplicateFilesTests.h"
//#include "ThreadingTests.hxx"		// include only in test projects to avoid the link dependency on Boost libraries in regular projects

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
		CEnvironmentTests::Instance();
		CLcsTests::Instance();
		CRegistryTests::Instance();
		CResequenceTests::Instance();
		CFmtUtilsTests::Instance();

		CPathTests::Instance();
		CPathGeneratorTests::Instance();
		CFileSystemTests::Instance();
		CTextFileIoTests::Instance();
		CStructuredStorageTest::Instance();
		CDuplicateFilesTests::Instance();

		// Threading tests are explicitly included only in the test projects DemoUtl and TesterUtlBase.
		// This is to avoid the link dependency on Boost libraries in regular projects.
		//CThreadingTests::Instance();
	}
}


#endif //USE_UT
