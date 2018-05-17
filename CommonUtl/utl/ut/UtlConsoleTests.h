#ifndef UtlConsoleTests_h
#define UtlConsoleTests_h
#pragma once

#include "StringTests.h"
#include "StringRangeTests.h"
#include "LcsTests.h"
#include "ResequenceTests.h"
#include "FileSystemTests.h"
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
		CLcsTests::Instance();
		CResequenceTests::Instance();
		CFileSystemTests::Instance();
		CPathGeneratorTests::Instance();
		//CThreadingTests::Instance();
	#endif
	}
}


#endif // UtlConsoleTests_h
