// Defines the entry point for the console application.

#include "stdafx.h"
#include "Application.h"
#include "CmdLineOptions.h"
#include "Directory.h"
#include "TreeGuides.h"
#include "utl/ConsoleApplication.h"
#include "utl/StdOutput.h"
#include "utl/StringUtilities.h"
#include "utl/TextEncoding.h"
#include <iostream>

#ifdef USE_UT
	#include "utl/MultiThreading.h"
	#include "utl/test/UtlConsoleTests.h"
	#include "test/TreePlusTests.h"
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
		CTreePlusTests::Instance();			// register application's tests
	#endif
	}
}


static const char s_helpMessage[] =
//	 |                       80 chars limit on the right mark                      |
	"Graphically displays the folder structure of a drive or path.\n"
	"Similar with the tree.com command with added flexibility.\n"
	"\n"
	"Written by Paul Cocoveanu, 2021.\n"
	"\n"
	"TreePlus [dir_path] [-f] [-h] [-ns] [-gs=G|A|B] [-l=N] [-max=FN] [-no] [-p]\n"
	"         [-e=ANSI|UTF8|UTF16]\n"
	"\n"
	"  dir_path\n"
	"      [drive:][path] - directory path to display.\n"
	"  out=<output_file>\n"
	"      Write output to text file using UTF8 encoding with BOM.\n"
	"  -f\n"
	"      Display the names of the files in each folder.\n"
	"  -h\n"
	"      Include hidden files.\n"
	"  -ns\n"
	"      No sorting of the directory and file names.\n"
	"  -gs=G|A|B\n"
	"      Sub-directory guides style, which could be one of:\n"
	"         G - Display graphical guides (default).\n"
	"         A - Display ASCII instead of graphical guides.\n"
	"         B - Display no guides, just blank spaces.\n"
	"  -l=N\n"
	"      Maximum depth level of the sub-directories displayed.\n"
	"  -max=FN\n"
	"      Maximum number of files to display in a directory.\n"
	"  -e=ANSI|UTF8|UTF16|UTF16-BE\n"
	"      If output redirected to a text file, encode the text file accordingly.\n"
	"        - ANSI encoding is the default.\n"
	"        - UTF8, UTF16 and UTF16-BE uses BOM (Byte Order Mark).\n"
	"  -no\n"
	"      No output (for performance profiling).\n"
	"  -p\n"
	"      Pause at completion.\n"
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
	void RunMain( std::wostream& os, const CCmdLineOptions& options ) throws_( std::exception, CException* )
	{
		CDirectory topDirectory( options );

		os << options.m_dirPath.GetPtr() << std::endl;			// print the root directory

		CTreeGuides guideParts( options.m_guidesProfileType, 4 );

		topDirectory.ListContents( os, guideParts );
		os.flush();			// just in case is using '\n' instead of std::endl
	}

	void RunTests( void )
	{
	#ifdef USE_UT
		st::CScopedInitializeOle scopedOle;		// some unit tests require OLE ()

		ut::RegisterAppUnitTests();
		ut::RunAllTests();
	#else
		ASSERT( false );		// no unit tests available
	#endif
	}
}


int _tmain( int argc, TCHAR* argv[] )
{
	CConsoleApplication application( io::Utf16 );		// output Unicode only to std::wcout, std::werr

	try
	{
		CCmdLineOptions options;
		options.ParseCommandLine( argc, argv );

		if ( options.HasOptionFlag( app::HelpMode ) )
			std::wcout << std::endl << s_helpMessage << std::endl;
		else if ( options.HasOptionFlag( app::UnitTestMode ) )
			app::RunTests();
		else
		{
			std::wstringstream os;
			app::RunMain( os, options );

			io::CStdOutput& rStdOutput = application.GetStdOutput();
			rStdOutput.Write( os.str(), options.m_fileEncoding );
		}

		if ( options.HasOptionFlag( app::PauseAtEnd ) )
			io::PressAnyKey();

		return 0;
	}
	catch ( const std::exception& exc )
	{
		app::ReportException( exc );
		return 1;
	}
}
