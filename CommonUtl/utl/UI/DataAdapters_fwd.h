#ifndef DataAdapters_fwd_h
#define DataAdapters_fwd_h
#pragma once

#include "utl/Range.h"


namespace ui
{
	enum StockValueFlags
	{
		LimitMinValue	= BIT_FLAG( 0 ),
		LimitMaxValue	= BIT_FLAG( 1 )
	};

	typedef int TStockValueFlags;
}


#endif // DataAdapters_fwd_h
