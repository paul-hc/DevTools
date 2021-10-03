// Defines the entry point for the console application.

#include "stdafx.h"
#include "Application.h"
#include "CmdLineOptions.h"
#include "ResourceFile.h"
#include "utl/ConsoleApplication.h"
#include "utl/StringUtilities.h"
#include "utl/TimeUtils.h"
#include <iostream>

#ifdef USE_UT
	#include "utl/MultiThreading.h"
	#include "utl/test/UtlConsoleTests.h"
	#include "test/ResourceFileTests.h"
#endif // USE_UT

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace ut
{
	void RegisterAppUnitTests( void )
	{
	#ifdef USE_UT
		ut::RegisterUtlConsoleTests();		// register all UTL console tests
		CResourceFileTests::Instance();		// register application's tests
	#endif
	}
}


static const char s_helpMessage[] =
//	 |                       80 chars limit on the right mark                      |
	"Stamps the VS_VERSION_INFO resource in an .rc file that contains the entry\n"
	"VALUE \"BuildTimestamp\" with the current date-time, preserving the original\n"
	".rc file timestamps (last modification time, access time).\n"
	"\n"
	"Written by Paul Cocoveanu, 2021.\n"
	"\n"
	"StampBuildVersion rc_file_path [timestamp] [-a] [-t]\n"
	"\n"
	"  rc_file_path\n"
	"      Path to the destination resource file in a Visual C++ project.\n"
	"      example:\n"
	"        'C:\\dev\\DevTools\\Wintruder\\Wintruder.rc'\n"
	"  timestamp\n"
	"      Optional, the timestamp to be stamped to the .rc file,\n"
	"      in 'DD-MM-YYYY H:mm:ss' format.\n"
	"      Example:\n"
	"        '16-09-2021 17:30:00'\n"
	"  -a\n"
	"      Add the \"BuildTimestamp\" entry into the VS_VERSION_INFO.\n"
	"  -t\n"
	"      Touch the target file: update the modification time on the target file\n"
	"      to force a project rebuild.\n"
	"  -? or -h\n"
	"      Display this help screen.\n"
#ifdef USE_UT
	"\n"
	"DEBUG BUILD:\n"
	"  -ut\n"
	"      Run the unit tests.\n"
#endif
	;


namespace app
{
	void RunMain( const CCmdLineOptions& options ) throws_( std::exception, CException* )
	{
		CResourceFile rcFile( options.m_targetRcPath, options.m_optionFlags );

		if ( rcFile.HasBuildTimestamp() )
		{
			rcFile.StampBuildTime( options.m_buildTimestamp );
			rcFile.Save();

			rcFile.Report( std::cout );
		}
	}
};


int _tmain( int argc, TCHAR* argv[] )
{
	CConsoleApplication application( io::Ansi );		// output only to std::cout, std::err

	try
	{
		CCmdLineOptions options;
		options.ParseCommandLine( argc, argv );

		if ( HasFlag( options.m_optionFlags, app::HelpMode ) )
			std::cout << std::endl << s_helpMessage << std::endl;
		else if ( HasFlag( options.m_optionFlags, app::UnitTestMode ) )
		{
		#ifdef USE_UT
			st::CScopedInitializeOle scopedOle;		// some unit tests require OLE ()

			ut::RegisterAppUnitTests();
			ut::RunAllTests();
		#else
			ASSERT( false );		// no unit tests available
		#endif
		}
		else
			app::RunMain( options );

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
