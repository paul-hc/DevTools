// Defines the entry point for the console application.

#include "pch.h"
#include "Options.h"
#include "utl/ConsoleApplication.h"
#include "utl/MultiThreading.h"				// for CScopedInitializeOle
#include "utl/test/Test.h"
#include "utl/test/ThreadingTests.hxx"		// include only in this test project to avoid the link dependency on Boost libraries in regular projects
#include <iostream>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#ifdef USE_UT
	#include "utl/test/UtlConsoleTests.h"
#endif


static const char s_helpMessage[] =
	"Executes the unit tests in UTL_BASE library.\n"
	"    Written by Paul Cocoveanu, 2023. Built on 23-Jan-2023.\n"
	"\n"
	"  /ut (default)\n"
	"    Run unit tests.\n"
	"  -? or -h\n"
	"    Display this help screen.\n"
	;


int _tmain( int argc, TCHAR* argv[] )
{
	CConsoleApplication application( io::Ansi );		// output only to std::cout, std::err

	try
	{
		COptions options;
		options.ParseCommandLine( argc, argv );

		if ( options.m_helpMode )
			std::cout << s_helpMessage << std::endl;
		else
		{
		#ifdef USE_UT
			std::clog << "Executing UTL_BASE unit tests:" << std::endl;

			st::CScopedInitializeOle scopedOle;		// some unit tests require OLE ()

			ut::RegisterUtlConsoleTests();
			CThreadingTests::Instance();			// special case: in this demo project we include the threading tests, with their dependency on Boos Threads library
			ut::RunAllTests();
		#else //USE_UT
			std::clog << "Heya! Do what in Release build?" << std::endl;
		#endif //USE_UT
		}
		return 0;
	}
	catch ( const std::exception& exc )
	{
		app::ReportException( exc );
		return 1;
	}
}
