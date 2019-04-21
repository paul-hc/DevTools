
#include "stdafx.h"

#ifdef _DEBUG		// no UT code in release builds
#include "UtlConsoleTests.h"
#include "StringTests.h"
#include "StringRangeTests.h"
#include "NumericTests.h"
#include "LcsTests.h"
#include "RegistryTests.h"
#include "ResequenceTests.h"
#include "FmtUtilsTests.h"
#include "FileSystemTests.h"
#include "PathTests.h"
#include "PathGeneratorTests.h"
#include "ThreadingTests.h"

#define new DEBUG_NEW


namespace ut
{
	void RegisterUtlConsoleTests( void )
	{
		// register UTL tests
		CStringTests::Instance();
		CStringRangeTests::Instance();
		CNumericTests::Instance();
		CLcsTests::Instance();
		CRegistryTests::Instance();
		CResequenceTests::Instance();
		CFmtUtilsTests::Instance();
		CFileSystemTests::Instance();
		CPathTests::Instance();
		CPathGeneratorTests::Instance();
		//CThreadingTests::Instance();
	}
}


#endif //_DEBUG
