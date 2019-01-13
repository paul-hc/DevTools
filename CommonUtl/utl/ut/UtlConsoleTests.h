#ifndef UtlConsoleTests_h
#define UtlConsoleTests_h
#pragma once

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


#endif // UtlConsoleTests_h
