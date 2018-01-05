// Copyleft 2004 Paul H. Cocoveanu
//
// TransferFiles.cpp : Defines the entry point for the console application.

#include "stdafx.h"
#include "XferOptions.h"
#include "FileTransfer.h"
#include "InputOutput.h"
#include "TestCase.h"
#include "utl/UnitTest.h"
#include <iostream>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


const TCHAR* pHelpMessage =
	_T("Copies files recursively in target directory tree (Unicode paths).\n")
	_T("    Written by Paul Cocoveanu, 2004-2016.\n")
	_T("\n")
	_T("xFer source_filter [target_dir]\n")
	_T("     [/a[[:]attributes]] [/d[:date]]\n")
	_T("     [/transfer:action]\n")
	_T("     [/exclude:file1[+file2][+file3]...]\n")
	_T("     [/ew:spec1[+spec2][+spec3]...]\n")
	_T("     [/q] [/jd] [/r] [/u] [/ud] [/ls or /lt] [/s[-]] [/y[-]]\n")
	_T("\n")
	_T("  source_filter\n")
	_T("      Specifies the source file(s) to transfer. Examples:\n")
	_T("        'C:\\Tools', 'C:\\Tools\\*.*', 'C:\\Tools\\*.bat;*.cmd'\n")
	_T("  target_dir\n")
	_T("      Specifies the relative location of the transfered files.\n")
	_T("  /a\n")
	_T("      Filter files with specified attributes:\n")
	_T("        D  Directories       R  Read-only files\n")
	_T("        H  Hidden files      A  Files ready for archiving\n")
	_T("        S  System files      -  Prefix meaning not\n")
	_T("  /d:m-d-y\n")
	_T("      Copies files changed on or after the specified date.\n")
	_T("      If no date is given, transfers only those files whose\n")
	_T("      source time is newer than the destination time.\n")
	_T("  /transfer\n")
	_T("      Specifies the file transfer action:\n")
	_T("        /t:c    /transfer:copy    Copy source files to target (default).\n")
	_T("        /t:m    /transfer:move    Move source files to target.\n")
	_T("        /t:r    /transfer:remove  Remove target files matching source files.\n")
	_T("  /exclude:file1[,file2][,file3]...\n")
	_T("      Specifies a list of files containing strings. When any of the\n")
	_T("      strings match any part of the absolute path of the file to be\n")
	_T("      transfered, that file will be excluded from being transfered.\n")
	_T("      For example, specifying a string like \\obj\\ or .obj will\n")
	_T("      exclude all files underneath the directory obj or all files\n")
	_T("      with the .obj extension respectively.\n")
	_T("  /ew:spec1[,spec2][,spec3]...\n")
	_T("      Specifies a list of file wildcard specs.\n")
	_T("      Matching files will be excluded from transfer.\n")
	_T("  /q  Quiet mode, does not display file names while transfering.\n")
	_T("  /jd Just creates directory structure, but does not transfer files.\n")
	_T("  /r  Overwrites read-only files.\n")
	_T("  /u  Copies only files that already exist in destination.\n")
	_T("  /ud Copies only files for which the destination directory exists.\n")
	_T("  /ls Just displays source files, i.e. files that would be transfered.\n")
	_T("  /lt Just displays target files, i.e. files that would be transfered to.\n")
	_T("  /s- Does not transfer sub-directories.\n")
	_T("  /y  Suppresses prompting to confirm you want to overwrite an\n")
	_T("      existing destination file.\n")
	_T("  /y- Causes prompting to confirm you want to overwrite an\n")
	_T("      existing destination file.\n")
	_T("  /?  Display this help screen.\n")
#ifdef _DEBUG
	_T("\n")
	_T("DEBUG BUILD:\n")
	_T("  /test\n")
	_T("      Run unit tests.\n")
	_T("  /debug\n")
	_T("      Break the program in debugger.\n")
	_T("  /ndebug [/nodebug]\n")
	_T("      Ignore, no debugging.\n")
#endif
;


int _tmain( int argc, TCHAR* argv[] )
{
	setlocale( LC_ALL, "" );
#ifdef _DEBUG
	// options to check outside the try block (debugging)
	ASSERT( !app::HasCommandLineOption( _T("debug") ) );

	if ( app::HasCommandLineOption( _T("test") ) )
	{
		&CTestCase::Instance();				// register TransferFiles tests

		std::cout << "RUNNING UNIT TESTS:" << std::endl;
		ut::CTestSuite::Instance().RunUnitTests();
		std::cout << "END UNIT TESTS - press ENTER... ";
		io::InputUserKey( false );
		std::cout << std::endl;
		return 0;
	}
#endif

	try
	{
		CXferOptions options;
		options.ParseCommandLine( argc, argv );
		options.CheckCreateTargetDirPath();

		CFileTransfer fileTransfer( options );
		fileTransfer.Run();
		fileTransfer.PrintStatistics( std::cout );
		return 0;
	}
	catch ( const std::exception& exc )
	{
		if ( !str::IsEmpty( exc.what() ) )
			std::cerr << exc.what() << std::endl;
		return 1;
	}
}
