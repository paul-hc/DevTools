#ifndef DibDraw_h
#define DibDraw_h
#pragma once

#include "DibDraw_fwd.h"


namespace gdi
{
	// image list drawing/stretching

	bool DrawImage( CDC* pDC, CImageList& rImageList, const CSize& imageSize, int index, const CPoint& pos, const CSize& size,
					UINT style = ILD_TRANSPARENT, COLORREF blendToColor = CLR_NONE );
	bool BlendImage( CDC* pDC, CImageList& rImageList, const CSize& imageSize, int index, const CPoint& pos, const CSize& size,
					 COLORREF blendToColor, BYTE toAlpha );

	void DrawDisabledImage( CDC* pDC, CImageList& rImageList, const CSize& imageSize, int index, const CPoint& pos, const CSize& size,
							UINT style = ILD_TRANSPARENT, COLORREF blendToColor = GetSysColor( COLOR_BTNFACE ), BYTE toAlpha = 64 );

	bool DrawEmbossedImage( CDC* pDC, CImageList& rImageList, const CSize& imageSize, int index, const CPoint& pos, const CSize& size,
							UINT style = ILD_TRANSPARENT, COLORREF blendToColor = GetSysColor( COLOR_BTNFACE ), bool scaleHighlight = true );

	enum Effect { Normal, Disabled, Embossed, Blend25, Blend50, Blend75 };

	void DrawImageEffect( CDC* pDC, CImageList& rImageList, const CSize& imageSize, int index, const CPoint& pos, const CSize& size, Effect effect,
						  UINT style = ILD_TRANSPARENT, COLORREF blendToColor = GetSysColor( COLOR_BTNFACE ) );


	// image list conversion

	bool MakeDisabledImageList( CImageList& rDestImageList, const CImageList& srcImageList, DisabledStyle style = gdi::DisabledBlendColor,
								COLORREF blendToColor = GetSysColor( COLOR_BTNFACE ), BYTE toAlpha = 128 );


	struct CImageInfo : public _IMAGEINFO
	{
		CImageInfo( const CImageList* pImageList, int index );

		void GetImageAt( int index ) { VERIFY( m_pImageList->GetImageInfo( index, this ) ); }

		// src DIB hbmImage is orientated vertically
		CRect MapSrcDibRect( int index );

		// dest DIB rect is orientated horizontally
		CRect GetDestRect( int index ) const { return CRect( CPoint( index * m_imageSize.cx, 0 ), m_imageSize ); }
	public:
		const CImageList* m_pImageList;
		UINT m_imageCount;
		CSize m_imageSize;
		CSize m_srcDibSize;
	};
}


#endif // DibDraw_h
