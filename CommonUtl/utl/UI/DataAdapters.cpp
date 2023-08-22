
#include "pch.h"
#include "DataAdapters.h"
#include "utl/StringUtilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace ui
{
	// CPositivePercentageAdapter implementation

	const CPositivePercentageAdapter* CPositivePercentageAdapter::Instance( void )
	{
		static const CPositivePercentageAdapter s_adapter;
		return &s_adapter;
	}


	// CZoomStockTags implementation

	CZoomStockTags::CZoomStockTags( void )
		: CStockTags<UINT>( CPositivePercentageAdapter::Instance(), _T("5|10|25|50|75|100|125|150|200|400|800|1200|1800|2400|3000") )
	{
		SetLimits( Range<UINT>( MinZoomPct, MaxZoomPct ), ui::LimitRange );
	}

	const CZoomStockTags* CZoomStockTags::Instance( void )
	{
		static CZoomStockTags s_zoomTags;
		return &s_zoomTags;
	}
}
