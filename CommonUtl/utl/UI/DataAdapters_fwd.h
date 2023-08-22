#ifndef DataAdapters_fwd_h
#define DataAdapters_fwd_h
#pragma once

#include "utl/Range.h"


namespace ui
{
	enum ValueLimitFlags
	{
		LimitMinValue	= BIT_FLAG( 0 ),
		LimitMaxValue	= BIT_FLAG( 1 ),

			LimitRange = LimitMinValue | LimitMaxValue
	};

	typedef int TValueLimitFlags;
}


#endif // DataAdapters_fwd_h
