#ifndef SampleView_fwd_h
#define SampleView_fwd_h
#pragma once


class CSampleView;


namespace ui
{
	interface ISampleCallback
	{
		virtual bool RenderSample( CDC* pDC, const CRect& boundsRect, CWnd* pCtrl ) = 0;

		// optional methods
		virtual bool RenderBackground( CDC* pDC, const CRect& boundsRect, CWnd* pCtrl ) { pDC, boundsRect, pCtrl; return false; }
		virtual void ShowPixelInfo( const CPoint& pos, COLORREF color, CWnd* pCtrl ) { pos, color, pCtrl; }
	};
}


#endif // SampleView_fwd_h
