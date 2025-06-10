#ifndef SampleView_fwd_h
#define SampleView_fwd_h
#pragma once


class CSampleView;


namespace ui
{
	interface ISampleCallback
	{
		virtual bool RenderSample( CDC* pDC, const CRect& boundsRect ) = 0;

		// optional methods
		virtual bool RenderBackground( CDC* pDC, const CRect& boundsRect ) { pDC, boundsRect; return false; }
		virtual void ShowPixelInfo( const CPoint& pos, COLORREF color ) { pos, color; }
	};
}


#endif // SampleView_fwd_h
