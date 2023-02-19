// Defines the entry point for the console application.

#include "stdafx.h"
#include "Application.h"
#include "CmdLineOptions.h"
#include "Directory.h"
#include "TreeGuides.h"
#include "utl/ConsoleApplication.h"
#include "utl/Guards.h"
#include "utl/StdOutput.h"
#include "utl/StringUtilities.h"
#include "utl/TextClipboard.h"
#include "utl/TextEncoding.h"
#include "utl/TextFileIo.h"
#include <iostream>

#ifdef USE_UT
	#include "utl/MultiThreading.h"
	#include "utl/test/UtlConsoleTests.h"
	#include "test/TreePlusTests.h"
#endif // USE_UT

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#include "utl/TextFileIo.hxx"


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
	"Graphically displays the folder structure of a drive or path,\n"
	"or converts tab-delimited text to tabbed tree-like hierarchy.\n"
	"Similar with the tree.com command with added flexibility.\n"
	"\n"
	"  Written by Paul Cocoveanu, 2021-2022 (built on 1 May 2022).\n"
	"\n"
	"TreePlus [dir_path] [-f] [-h] [-ns] [-gs=G|A|B|T[-]] [-l=N] [-max=FN] [-p]\n"
	"         [-e=ANSI|UTF8|UTF16] [-no] [-t]\n"
	"\n"
	"  - or with tab-delimited text:\n"
	"TreePlus in=<table_input_file> [out=<output_file>] [-ns]\n"
	"TreePlus in_clip out_clip [-ns]\n"
	"\n"
	"  dir_path\n"
	"      [drive:][path] - directory path to display.\n"
	"  in=<table_input_file>\n"
	"      Read tab-delimited text from the input file.\n"
	"      No dir_path, use tab guides by default (option '-gs=T-').\n"
	"  out=<output_file>\n"
	"      Write output to text file using UTF8 encoding with BOM.\n"
	"  in_clip\n"
	"      Paste input tab-delimited text from clipboard\n"
	"      (similar with 'in=path' switch).\n"
	"  out_clip\n"
	"      Copy output to clipboard (similar with 'out=path' switch).\n"
	"  -f\n"
	"      Display the names of the files in each folder.\n"
	"  -h\n"
	"      Include hidden files.\n"
	"  -ns\n"
	"      No sorting of the directory and file names, or tab-delimited text.\n"
	"  -gs=G|A|B|T\n"
	"      Sub-directory guides style, which could be one of:\n"
	"         G - Display graphical guides (default).\n"
	"         A - Display ASCII instead of graphical guides.\n"
	"         B - Display no guides, just blank spaces.\n"
	"         T - Display tab guides.\n"
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
	"  -t\n"
	"      Display execution timing statistics.\n"
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
	void RunMain( std::wstringstream& os, const CCmdLineOptions& options ) throws_( std::exception, CException* )
	{
		CDirectory topDirectory( options );

		if ( !options.HasOptionFlag( app::TableInputMode ) )
			os << options.m_dirPath.GetPtr() << std::endl;			// print the root directory

		CTreeGuides guideParts( options.m_guidesProfileType, 4 );

		topDirectory.ListContents( os, guideParts );
		os.flush();			// just in case is using '\n' instead of std::endl

		if ( options.HasOptionFlag( app::ClipboardOutputMode ) )
		{
			CTextClipboard::CMessageWnd msgWnd;		// use a temporary message-only window for clipboard copy
			if ( !CTextClipboard::CopyText( os.str(), msgWnd.GetWnd() ) )
				throw CRuntimeException( _T("Cannot copy the output to clipboard") );
		}
		else if ( !options.m_outputFilePath.IsEmpty() )
			io::WriteStringToFile( options.m_outputFilePath, os.str(), options.m_fileEncoding );
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
			utl::CMultiStageTimer timer;

			std::wstringstream os;
			app::RunMain( os, options );
			timer.AddStage( _T("Main execution") );

			std::wstring outcome = os.str();

			if ( options.HasOptionFlag( app::ClipboardOutputMode ) )
				outcome = str::Format( _T("Output copied to clipboard.\n") );
			else if ( options.HasOptionFlag( app::ClipboardOutputMode ) )
				outcome = str::Format( _T("Output written to text file: %s\n"), options.m_outputFilePath.GetPtr() );

			io::CStdOutput& rStdOutput = application.GetStdOutput();
			rStdOutput.Write( outcome, options.m_fileEncoding );
			timer.AddStage( _T("Output execution") );

			if ( options.HasOptionFlag( app::ShowExecTimeStats ) )
			{
				timer.AddCheckpoint( _T("Search for files"), CDirectory::GetTotalElapsedEnum() );
				timer.Report( std::wclog << std::endl );
			}
		}

		if ( options.HasOptionFlag( app::PauseAtEnd ) )
			io::PressAnyKey();
	}
	catch ( const std::exception& exc )
	{
		CAppTools::AddMainResultError();
		app::ReportException( exc );
	}

	return CAppTools::GetMainResultCode();			// count of tests that failed
}
