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
		ClipboardOutputMode	= BIT_FLAG( 2 ),		// tab-delimited input text file (no directory enumeration)
		UnitTestMode		= BIT_FLAG( 3 ),
		ShowExecTimeStats	= BIT_FLAG( 4 ),
		PauseAtEnd			= BIT_FLAG( 5 ),

		DisplayFiles		= BIT_FLAG( 16 ),
		SkipFileGroupLine	= BIT_FLAG( 17 ),		// skip line separator after a group of files (leafs) - disabled for TabGuides, which is tab-separated
		ShowHiddenNodes		= BIT_FLAG( 18 ),
		NoSorting			= BIT_FLAG( 19 ),		// display the files (tab-delimited input) in the existing order
		NoOutput			= BIT_FLAG( 20 ),		// for timing just the file enumeration (no std output)
	};

	typedef utl::CFlagSet<Option> TOption;
}


enum GuidesProfileType { GraphGuides, AsciiGuides, BlankGuides, TabGuides,  _ProfileCount };


#endif // CmdLineOptions_fwd_h
