// Defines the entry point for the console application.

#include "stdafx.h"
#include "CmdLineOptions.h"
#include "ResourceFile.h"
#include "utl/ConsoleApplication.h"
#include "utl/StringUtilities.h"
#include "utl/TimeUtils.h"
#include <iostream>

#ifdef _DEBUG
//#define USE_UT

#ifdef USE_UT
	#include "utl/MultiThreading.h"
	#include "utl/test/UtlConsoleTests.h"
	#include "test/TransferFuncTests.h"
#endif // USE_UT

#endif // _DEBUG

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace ut
{
	void RegisterAppUnitTests( bool debugChildProcs )
	{
	#ifdef USE_UT
		ut::RegisterUtlConsoleTests();
		CTransferFuncTests::Instance().SetDebugChildProcs( debugChildProcs );		// register TransferFiles tests
	#else
		debugChildProcs;
	#endif
	}
}


static const char s_helpMessage[] =
	// 80 chars limit on the right mark                                            |
	"Stamps the VS_VERSION_INFO resource in an .rc file that contains the entry\n"
	"VALUE \"BuildTimestamp\" with the current date-time, preserving the original\n"
	".rc file timestamps (last modification time, access time).\n"
	"\n"
    "Written by Paul Cocoveanu, 2021.\n"
	"\n"
	"StampBuildVersion rc_file_path [timestamp]\n"
	"\n"
	"  rc_file_path\n"
	"      Path to the destination resource file in a Visual C++ project.\n"
	"      Examples:\n"
	"        'C:\\dev\\DevTools\\Wintruder\\Wintruder.rc'\n"
	"  timestamp\n"
	"      Optional, the timestamp to be stamped to the .rc file,\n"
	"      in 'DD-MM-YYYY H:mm:ss' format.\n"
	"      Examples:\n"
	"        '16-09-2021 17:30:00\n"
	"  /? or /h\n"
	"      Display this help screen.\n"
#ifdef USE_UT
	"\n"
	"DEBUG BUILD:\n"
	"  /ut\n"
	"      Run unit tests.\n"
#endif
	;


int _tmain( int argc, TCHAR* argv[] )
{
	CConsoleApplication application;

#ifdef USE_UT
	// options to check outside the try block (debugging)
	ASSERT( !app::HasCommandLineOption( _T("debug") ) );

	std::tstring value;
	if ( app::HasCommandLineOption( _T("ut"), &value ) )
	{
		st::CScopedInitializeOle scopedOle;		// some unit tests require OLE ()

		ut::RegisterAppUnitTests( value == _T("debug") );
		ut::RunAllTests();
		return 0;
	}
#endif

	try
	{
		CCmdLineOptions options;
		options.ParseCommandLine( argc, argv );

		if ( options.m_helpMode )
			std::cout << s_helpMessage << std::endl;
		else
		{
			CResourceFile rcFile( options.m_targetRcPath );

			if ( rcFile.HasBuildTimestamp() )
			{
				rcFile.StampBuildTime( options.m_buildTimestamp );
				rcFile.Save();

				rcFile.Report( std::cout );
			}
		}
		return 0;
	}
	catch ( const std::exception& exc )
	{
		app::ReportException( exc );
		return 1;
	}
	catch ( CException* pExc )
	{
		app::ReportException( pExc );
		pExc->Delete();
		return 1;
	}
}
