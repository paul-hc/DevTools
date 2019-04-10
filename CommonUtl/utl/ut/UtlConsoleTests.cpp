
#include "stdafx.h"
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

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace ut
{
	void RegisterUtlConsoleTests( void )
	{
	#ifdef _DEBUG
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
	#endif
	}
}
