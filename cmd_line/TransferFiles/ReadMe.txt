

*.mp3;*.m4?;*.mp4 "%TargetDir%" /transfer:move

debug (old):
Command: $(TargetPath)
Command arguments: "E:\Media Library\audio\other\samples\FZ-UTF8\*.txt" "E:\Media Library\audio\other\samples\FZ-UTF8@LOSSY" /y



E:\MyProjects\Tools\##\SOURCE\*.h,*.cpp E:\MyProjects\Tools\##\TARGET\ /LT /D:11-10-2004

		file::MakeDirectoryPath( "E:\\MyProjects\\Tools\\##\\TARGET\\Dir1\\Dir2\\Dir3" );
		file::MakeDirectoryPath( "\\\\Medex\\Perl\\bin\\1\\2\\3\\" );
		file::MakeDirectoryPath( "..\\a\\b\\c" );

#include <iostream>

	std::cout << "drive=" << drive << std::endl;
	std::cout << "dir=" << dir << std::endl;
	std::cout << "fNameExt=" << fNameExt << std::endl;



#include <stdio.h>
#include <conio.h>
#include <stdlib.h>
#include <direct.h>

char full[_MAX_PATH], part[_MAX_PATH];

void fpath( void )
{
   while( 1 )
   {
      printf( "Enter partial path or SPACE+ENTER to quit: " );
      gets( part );
      if( std::tstring( part ) == _T(" ") )
         break;

      if( _fullpath( full, part, _MAX_PATH ) != nullptr )
         printf( "Full path is: %s\n", full );
      else
         printf( "Invalid path\n" );
   }
}


========================================================================
       CONSOLE APPLICATION : TransferFiles
========================================================================


AppWizard has created this TransferFiles application for you.

This file contains a summary of what you will find in each of the files that
make up your TransferFiles application.

TransferFiles.dsp
    This file (the project file) contains information at the project level and
    is used to build a single project or subproject. Other users can share the
    project (.dsp) file, but they should export the makefiles locally.

TransferFiles.cpp
    This is the main application source file.

TransferFiles.rc
    This is a listing of all of the Microsoft Windows resources that the
    program uses.  It includes the icons, bitmaps, and cursors that are stored
    in the RES subdirectory.  This file can be directly edited in Microsoft
    Visual C++.

/////////////////////////////////////////////////////////////////////////////
Other standard files:

StdAfx.h, StdAfx.cpp
    These files are used to build a precompiled header (PCH) file
    named TransferFiles.pch and a precompiled types file named StdAfx.obj.

Resource.h
    This is the standard header file, which defines new resource IDs.
    Microsoft Visual C++ reads and updates this file.

/////////////////////////////////////////////////////////////////////////////
Other notes:

AppWizard uses "TODO:" to indicate parts of the source code you
should add to or customize.

/////////////////////////////////////////////////////////////////////////////

	if ( m_copyOptions.m_transferMode == ExecuteTransfer )
	{	// Do the actual file transfer
		if ( m_copyOptions.m_displayFileNames )
			std::for_each( m_nodesToTransfer.begin(), m_nodesToTransfer.end(), func::PtrStreamInserter( std::cout, "\n" ) );
	}
	else
	{	// just display SOURCE or TARGET
		std::for_each( m_nodesToTransfer.begin(), m_nodesToTransfer.end(),
					   JustDisplayFile( m_copyOptions.m_transferMode, std::cout, "\n" ) );
	}
