#ifndef CmdLineOptions_fwd_h
#define CmdLineOptions_fwd_h
#pragma once


namespace app
{
	enum Option
	{
		HelpMode			= BIT_FLAG( 0 ),
		UnitTestMode		= BIT_FLAG( 1 ),
		Add_BuildTimestamp	= BIT_FLAG( 2 )
	};
	typedef int TOption;
}


#endif // CmdLineOptions_fwd_h
