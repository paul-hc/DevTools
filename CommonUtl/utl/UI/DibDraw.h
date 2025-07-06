#ifndef DibDraw_h
#define DibDraw_h
#pragma once

#include "Image_fwd.h"
#include "DibDraw_fwd.h"


namespace gdi
{
	// image list drawing/stretching

	bool DrawImage( CDC* pDC, CImageList& rImageList, const CSize& imageSize, int index, const CPoint& pos, const CSize& size,
					UINT style = ILD_TRANSPARENT, COLORREF blendToColor = CLR_NONE );
	bool BlendImage( CDC* pDC, CImageList& rImageList, const CSize& imageSize, int index, const CPoint& pos, const CSize& size,
					 COLORREF blendToColor, BYTE toAlpha );

	void DrawDisabledImage( CDC* pDC, CImageList& rImageList, const CSize& imageSize, int index, const CPoint& pos, const CSize& size,
							UINT style = ILD_TRANSPARENT, COLORREF blendToColor = ::GetSysColor( COLOR_BTNFACE ), BYTE toAlpha = 64 );

	bool DrawEmbossedImage( CDC* pDC, CImageList& rImageList, const CSize& imageSize, int index, const CPoint& pos, const CSize& size,
							UINT style = ILD_TRANSPARENT, COLORREF blendToColor = ::GetSysColor( COLOR_BTNFACE ), bool scaleHighlight = true );

	enum Effect { Normal, Disabled, Embossed, Blend25, Blend50, Blend75 };

	void DrawImageEffect( CDC* pDC, CImageList& rImageList, const CSize& imageSize, int index, const CPoint& pos, const CSize& size, Effect effect,
						  UINT style = ILD_TRANSPARENT, COLORREF blendToColor = ::GetSysColor( COLOR_BTNFACE ) );


	// image list conversion

	bool MakeDisabledImageList( CImageList* pDestImageList, const CImageList& srcImageList, DisabledStyle style = gdi::Dis_FadeGray );
}


#endif // DibDraw_h
