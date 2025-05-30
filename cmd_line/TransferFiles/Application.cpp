// Defines the entry point for the console application.

#include "pch.h"
#include "XferOptions.h"
#include "FileTransfer.h"
#include "utl/ConsoleApplication.h"
#include "utl/MultiThreading.h"
#include "utl/StringUtilities.h"
#include <iostream>

#ifdef _DEBUG
#include "utl/test/UtlConsoleTests.h"
#include "test/TransferFuncTests.h"
#endif

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace ut
{
	void RegisterAppUnitTests( bool debugChildProcs )
	{
	#ifdef _DEBUG
		ut::RegisterUtlConsoleTests();
		CTransferFuncTests::Instance().SetDebugChildProcs( debugChildProcs );		// register TransferFiles tests
	#else
		debugChildProcs;
	#endif
	}
}


static const char s_helpMessage[] =
	"Copies files recursively in target directory tree (Unicode paths).\n"
	"    Written by Paul Cocoveanu, 2004-2022. Built on 12 June 2022.\n"
	"\n"
	"xFer source_filter [target_dir]\n"
	"     [/transfer=action]\n"
	"     [/bk[=bk_dir]]\n"
	"     [/ch[=CRC32]]\n"
	"     [/a[=attributes]] [/d[=date]]\n"
	"     [/exclude=file1[+file2][+file3]...]\n"
	"     [/ew=spec1[+spec2][+spec3]...]\n"
	"     [/q] [/jd] [/r] [/u] [/ud] [/ls or /lt] [/s[-]] [/y[-]]\n"
	"\n"
	"  source_filter\n"
	"      Specifies the source file(s) to transfer. Examples:\n"
	"        'C:\\Tools', 'C:\\Tools\\*.*', 'C:\\Tools\\*.bat;*.cmd'\n"
	"  target_dir\n"
	"      Specifies the relative location of the transfered files.\n"
	"\n"
	"  /transfer\n"
	"      Specifies the file transfer action:\n"
	"        /t=c    /transfer=copy    Copy source files to target (default).\n"
	"        /t=m    /transfer=move    Move source files to target.\n"
	"        /t=r    /transfer=remove  Remove target files matching source files.\n"
	"  /bk[=bk_dir]\n"
	"      Backup destination files about to be overwritten using a numeric suffix.\n"
	"      If bk_dir is specified, all backups go in that directory,\n"
	"      relative to target_dir, otherwise uses target_dir.\n"
	"  /ch[=size|crc32]\n"
	"      Transfer only source files that are newer, or optionally have a different\n"
	"      content than the existing target file:\n"
	"        size    compared by file size;\n"
	"        crc32   compared by file size and CRC32 checksum (slower).\n"
	"  /a[=attributes]\n"
	"      Filter files with specified attributes:\n"
	"        D  Directories       R  Read-only files\n"
	"        H  Hidden files      A  Files ready for archiving\n"
	"        S  System files      -  Prefix meaning not\n"
	"  /d=m-d-y\n"
	"      Copies files changed on or after the specified date.\n"
	"      If no date is given, transfers only those files whose\n"
	"      source time is newer than the destination time.\n"
	"  /exclude=file1[,file2][,file3]...\n"
	"      Specifies a list of files containing strings. When any of the\n"
	"      strings match any part of the absolute path of the file to be\n"
	"      transfered, that file will be excluded from being transfered.\n"
	"      For example, specifying a string like \\obj\\ or .obj will\n"
	"      exclude all files underneath the directory obj or all files\n"
	"      with the .obj extension respectively.\n"
	"  /ew=spec1[,spec2][,spec3]...\n"
	"      Specifies a list of file wildcard specs.\n"
	"      Matching files will be excluded from transfer.\n"
	"  /q  Quiet mode, does not display file names while transfering.\n"
	"  /jd Just creates directory structure, but does not transfer files.\n"
	"  /r  Overwrites read-only files.\n"
	"  /u  Copies only files that already exist in destination.\n"
	"  /ud Copies only files for which the destination directory exists.\n"
	"  /ls Just displays source files, i.e. files that would be transfered.\n"
	"  /lt Just displays target files, i.e. files that would be transfered to.\n"
	"  /s- Does not transfer sub-directories.\n"
	"  /y  Non-interactive mode: suppress prompting to confirm you want to overwrite\n"
	"      an existing destination file.\n"
	"  /y- Causes prompting to confirm you want to overwrite an\n"
	"      existing destination file.\n"
	"  /?  Display this help screen.\n"
#ifdef _DEBUG
	"\n"
	"DEBUG BUILD:\n"
#ifdef USE_UT
	"  /ut[=debug]\n"
	"      Run unit tests.\n"
	"      Break the child processes in the debugger if '=debug' is specified.\n"
#endif //USE_UT
	"  /debug\n"
	"      Break the program in debugger.\n"
	"  /ndebug [/nodebug]\n"
	"      Ignore, no debugging (for testing).\n"
#endif //_DEBUG
	;


int _tmain( int argc, TCHAR* argv[] )
{
	CConsoleApplication application( io::Ansi );		// output only to std::cout, std::err

#ifdef _DEBUG
	// options to check outside the try block (debugging)
	ASSERT( !app::HasCommandLineOption( _T("debug") ) );

	std::tstring value;
	if ( app::HasCommandLineOption( _T("ut"), &value ) )
	{
		st::CScopedInitializeOle scopedOle;		// some unit tests require OLE ()

		ut::RegisterAppUnitTests( value == _T("debug") );
		ut::RunAllTests();
		return CAppTools::GetMainResultCode();			// count of tests that failed
	}
#endif

	try
	{
		CXferOptions options;
		options.ParseCommandLine( argc, argv );

		if ( options.m_helpMode )
			std::cout << s_helpMessage << std::endl;
		else
		{
			CFileTransfer transfer( &options );
			transfer.Run();
			transfer.PrintStatistics( std::cout );
		}
	}
	catch ( const std::exception& exc )
	{
		CAppTools::AddMainResultError();

		app::ReportException( exc );
	}

	return CAppTools::GetMainResultCode();			// count of tests that failed
}
