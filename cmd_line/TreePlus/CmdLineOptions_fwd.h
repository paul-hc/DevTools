#ifndef CmdLineOptions_fwd_h
#define CmdLineOptions_fwd_h
#pragma once


namespace app
{
	enum Option
	{
		HelpMode			= BIT_FLAG( 0 ),
		UnitTestMode		= BIT_FLAG( 1 ),
		PauseAtEnd			= BIT_FLAG( 2 ),

		DisplayFiles		= BIT_FLAG( 8 ),
		NoSorting			= BIT_FLAG( 9 ),		// display the files in the existing order
		NoOutput			= BIT_FLAG( 10 )		// for timing just the file enumeration (no std output)
	};
	typedef int TOption;
}


#endif // CmdLineOptions_fwd_h
