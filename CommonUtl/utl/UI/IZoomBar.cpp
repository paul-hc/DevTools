
#include "pch.h"
#include "IZoomBar.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace ui
{
	enum ZoomLimits { MinZoomPct = 1, MaxZoomPct = 10000 };

	CStdZoom::CStdZoom( void )
		: m_limits( MinZoomPct, MaxZoomPct )
	{
		static const UINT s_zoomPcts[] = { 5, 10, 25, 50, 75, 100, 125, 150, 200, 400, 800, 1200, 1800, 2400, 3000 };
		m_zoomPcts.assign( s_zoomPcts, END_OF( s_zoomPcts ) );
	}

	CStdZoom& CStdZoom::Instance( void )
	{
		static CStdZoom s_stdZoomInfo;
		return s_stdZoomInfo;
	}
}
