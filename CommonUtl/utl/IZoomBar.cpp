
#include "stdafx.h"
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
		static const UINT zoomPcts[] = { 5, 10, 25, 50, 75, 100, 125, 150, 200, 400, 800, 1200, 1800, 2400, 3000 };
		m_zoomPcts.assign( zoomPcts, zoomPcts + COUNT_OF( zoomPcts ) );
	}

	CStdZoom& CStdZoom::Instance( void )
	{
		static CStdZoom stdZoomInfo;
		return stdZoomInfo;
	}
}
