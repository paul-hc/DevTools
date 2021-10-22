#ifndef CmdLineOptions_fwd_h
#define CmdLineOptions_fwd_h
#pragma once

#include "utl/FlagSet.h"


namespace app
{
	enum Option
	{
		HelpMode			= BIT_FLAG( 0 ),
		UnitTestMode		= BIT_FLAG( 1 ),
		ShowExecTimeStats	= BIT_FLAG( 2 ),
		PauseAtEnd			= BIT_FLAG( 3 ),

		DisplayFiles		= BIT_FLAG( 8 ),
		ShowHiddenNodes		= BIT_FLAG( 9 ),
		NoSorting			= BIT_FLAG( 10 ),		// display the files in the existing order
		NoOutput			= BIT_FLAG( 11 )		// for timing just the file enumeration (no std output)
	};

	typedef utl::CFlagSet<Option> TOption;
}


enum GuidesProfileType { GraphGuides, AsciiGuides, BlankGuides,  _ProfileCount };


#endif // CmdLineOptions_fwd_h
