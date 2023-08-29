
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


	// CDurationSecondsStockTags implementation

	const ui::CNumericUnitAdapter<double> CDurationSecondsStockTags::s_secondsAdapter( _T(" sec") );

	CDurationSecondsStockTags::CDurationSecondsStockTags( void )
		: CStockTags<double>( &s_secondsAdapter, _T("0.1|0.25|0.5|0.75|1|1.5|2|3|4|5|8|10|12|15|17|20|25|30") )
	{
		Range<double> limits( FromMiliseconds( USER_TIMER_MINIMUM ), FromMiliseconds( USER_TIMER_MAXIMUM ) );

		SetLimits( limits, ui::LimitRange );
	}

	const CDurationSecondsStockTags* CDurationSecondsStockTags::Instance( void )
	{
		static const CDurationSecondsStockTags s_stockTags;
		return &s_stockTags;
	}
}
