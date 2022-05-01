#ifndef CmdLineOptions_fwd_h
#define CmdLineOptions_fwd_h
#pragma once

#include "utl/FlagSet.h"


namespace app
{
	enum Option
	{
		HelpMode			= BIT_FLAG( 0 ),
		TableInputMode		= BIT_FLAG( 1 ),		// tab-delimited input text file (no directory enumeration)
		UnitTestMode		= BIT_FLAG( 2 ),
		ShowExecTimeStats	= BIT_FLAG( 3 ),
		PauseAtEnd			= BIT_FLAG( 4 ),

		DisplayFiles		= BIT_FLAG( 8 ),
		SkipFileGroupLine	= BIT_FLAG( 9 ),		// skip line separator after a group of files (leafs) - disabled for TabGuides, which is tab-separated
		ShowHiddenNodes		= BIT_FLAG( 10 ),
		NoSorting			= BIT_FLAG( 11 ),		// display the files in the existing order
		NoOutput			= BIT_FLAG( 12 )		// for timing just the file enumeration (no std output)
	};

	typedef utl::CFlagSet<Option> TOption;
}


enum GuidesProfileType { GraphGuides, AsciiGuides, BlankGuides, TabGuides,  _ProfileCount };


#endif // CmdLineOptions_fwd_h
