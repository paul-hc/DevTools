// Defines the entry point for the console application.

#include "stdafx.h"
#include "HexDump.h"
#include "StdOutput.h"
#include <string>
#include <iostream>
#include <sstream>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace app
{
	std::string s_filePath;
	bool s_helpMode = false;

	void ParseCommandLine( int argc, char* argv[] )
	{
		for ( int i = 1; i != argc; ++i )
		{
			const char* pArg = argv[ i ];

			switch ( pArg[ 0 ] )
			{
				case '/':
				case '-':
					if ( '\0' == pArg[ 2 ] )
						switch ( tolower( pArg[ 1 ] ) )
						{
							case 'h':
							case '?':
								s_helpMode = true;
								continue;
						}

					break;
				default:
					if ( s_filePath.empty() )
					{
						s_filePath = pArg;
						continue;
					}
			}

			throw std::invalid_argument( std::string( "Unknown argument: " ) + pArg );
		}

		if ( !s_helpMode && s_filePath.empty() )
			s_helpMode = true;
	}


	static const char s_helpMessage[] =
	//	 |                       80 chars limit on the right mark                      |
		"Display the binary contents of a file as hexadecimal bytes and characters.\n"
		"\n"
		"Written by Paul Cocoveanu, 2021.\n"
	#ifdef _DEBUG
		"  DEBUG BUILD!\n"
	#endif
		"\n"
		"HexDump <file_path> [-?]\n"
		"\n"
		"  file_path\n"
		"      Path of the file to display.\n"
		"  -? or -h\n"
		"      Display this help screen.\n"
		;
}


int main( int argc, char* argv[] )
{
	try
	{
		app::ParseCommandLine( argc, argv );

		if ( app::s_helpMode )
			std::cout << std::endl << app::s_helpMessage << std::endl;
		else
		{	// optimize speed: bypass std::cout output, by writing to a string stream and efficiently writing the ouput to the console/redirected output
			std::ostringstream os;
			io::HexDump( os, app::s_filePath.c_str() );

			io::CStdOutput stdOutput;
			stdOutput.Write( os.str() );
		}

		return 0;
	}
	catch ( const std::exception& exc )
	{
		std::cerr << "Error: " << exc.what() << '!' << std::endl;
		return 1;
	}
}

