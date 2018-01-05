#ifndef IZoomBar_h
#define IZoomBar_h
#pragma once

#include "Image_fwd.h"			// ui::AutoImageSize
#include "Range.h"
#include "ui_fwd.h"


namespace ui
{
	interface IZoomBar
	{
		virtual bool OutputAutoSize( ui::AutoImageSize autoImageSize ) = 0;
		virtual ui::AutoImageSize InputAutoSize( void ) const = 0;

		virtual bool OutputZoomPct( UINT zoomPct ) = 0;
		virtual UINT InputZoomPct( ui::ComboField byField ) const = 0;		// return 0 on error
	};


	struct CStdZoom
	{
	private:
		CStdZoom( void );
	public:
		static CStdZoom& Instance( void );
	public:
		std::vector< UINT > m_zoomPcts;
		Range< UINT > m_limits;
	};
}


#endif // IZoomBar_h
