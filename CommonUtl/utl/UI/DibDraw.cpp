
#include "pch.h"
#include "DibDraw.h"
#include "DibSection.h"
#include "DibPixels.h"
#include "GdiCoords.h"
#include "ImageProxies.h"
#include "EnumTags.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#include "Image_fwd.hxx"


namespace gdi
{
	const CEnumTags& GetTags_DisabledStyle( void )
	{
		static const CEnumTags s_tags( _T("Dis_FadeGray|Dis_GrayScale|Dis_GrayOut|Dis_DisabledEffect|Dis_BlendColor|Dis_MfcStd") );
		return s_tags;
	}
}


namespace gdi
{
	COLORREF MakeKeyColor( COLORREF color );

	bool DrawImage( CDC* pDC, CImageList& rImageList, const CSize& imageSize, int index, const CPoint& pos, const CSize& size,
					UINT style /*= ILD_TRANSPARENT*/, COLORREF blendToColor /*= CLR_NONE*/ )
	{
		ASSERT_PTR( rImageList.GetSafeHandle() );
		ASSERT( imageSize.cx > 0 && imageSize.cy > 0 );

		if ( size != imageSize )
			SetFlag( style, ILD_SCALE );		// stretch instead of clip

		return rImageList.DrawEx( pDC, index, pos, size, CLR_NONE, blendToColor, style ) != FALSE;
	}

	// smooth blending for all bpps, compared with DrawImage( ..., ILD_BLEND50 )
	bool BlendImage( CDC* pDC, CImageList& rImageList, const CSize& imageSize, int index, const CPoint& pos, const CSize& size,
					 COLORREF blendToColor, BYTE toAlpha )
	{
		CDibSection dib;
		CDibPixels pixels;

		if ( !dib.CreateDIBSection( &pixels, imageSize.cx, imageSize.cy, 32 ) )		// 32 bpp
			return false;

		bool hasAlpha = gdi::HasAlphaTransparency( rImageList, index );
		if ( !hasAlpha )
		{
			blendToColor = MakeKeyColor( blendToColor );		// make blendToColor "unique" so that we don't replace other occurences in the image
			pixels.Fill( blendToColor );
		}

		CScopedBitmapMemDC scopedBitmap( &dib );
		CDC* pMemDC = dib.GetBitmapMemDC();

		DrawImage( pMemDC, rImageList, imageSize, index, CPoint( 0, 0 ), imageSize );
		::GdiFlush();

		if ( !hasAlpha )
			pixels.ForEach( func::ReplaceColor( blendToColor, color::Black, 0 ) );		// fillColor -> transparent (0 alpha, i.e. opacity)

		pixels.ApplyBlendColor( blendToColor, toAlpha );
		return CDibSection::AlphaBlended == dib.DrawTransparent( pDC, CRect( pos, size ) );
	}

	void DrawDisabledImage( CDC* pDC, CImageList& rImageList, const CSize& imageSize, int index, const CPoint& pos, const CSize& size,
							UINT style /*= ILD_TRANSPARENT*/, COLORREF blendToColor /*= ::GetSysColor( COLOR_BTNFACE )*/, BYTE toAlpha /*= 64*/ )
	{
		CDibSection dib;
		CDibPixels pixels;

		if ( dib.CreateDIBSection( &pixels, imageSize.cx, imageSize.cy, 32 ) )		// 32 BPP
		{
			bool hasAlpha = gdi::HasAlphaTransparency( rImageList, index );
			if ( !hasAlpha )
			{
				blendToColor = MakeKeyColor( blendToColor );		// make blendToColor "unique" so that we don't replace other occurences in the image
				pixels.Fill( blendToColor );
			}

			CScopedBitmapMemDC scopedBitmap( &dib );
			CDC* pMemDC = dib.GetBitmapMemDC();

			DrawImage( pMemDC, rImageList, imageSize, index, CPoint( 0, 0 ), imageSize, style );
			::GdiFlush();

			if ( !hasAlpha )
				pixels.ForEach( func::ReplaceColor( blendToColor, color::Black, 0 ) );		// fillColor -> transparent (0 alpha, i.e. opacity)

			pixels.ApplyDisabledGrayOut( blendToColor, toAlpha );
			if ( CDibSection::AlphaBlended == dib.DrawTransparent( pDC, CRect( pos, size ) ) )
				return;
		}
		DrawEmbossedImage( pDC, rImageList, imageSize, index, pos, size, style, blendToColor );
	}

	// use the standard embossing
	//
	bool DrawEmbossedImage( CDC* pDC, CImageList& rImageList, const CSize& imageSize, int index, const CPoint& pos, const CSize& size,
							UINT style /*= ILD_TRANSPARENT*/, COLORREF blendToColor /*= ::GetSysColor( COLOR_BTNFACE )*/, bool scaleHighlight /*= true*/ )
	{
		//	black: pattern
		//	white: destintation (transparent)
		CDC memDC;
		if ( !memDC.CreateCompatibleDC( pDC ) )
			return false;

		int oldStretchBltMode = pDC->SetStretchBltMode( COLORONCOLOR );

		CBitmap maskBitmap;
		maskBitmap.CreateBitmap( imageSize.cx, imageSize.cy, 1, 1, nullptr );		// for color: maskBitmap.CreateCompatibleBitmap( pDC, cx, cy );

		CScopedGdi<CBitmap> scopedBitmap( &memDC, &maskBitmap );

		memDC.PatBlt( 0, 0, imageSize.cx, imageSize.cy, WHITENESS );		// fill background white first
		DrawImage( &memDC, rImageList, imageSize, index, CPoint( 0, 0 ), imageSize, style | ILD_BLEND50, blendToColor );		// 50% blending

		COLORREF oldBkColor = pDC->SetBkColor( color::White );

		// draw with highlight color at offset (1, 1) - this will not be visible on white background
		CSize delta( 1, 1 );
		if ( scaleHighlight )
			delta = ui::ScaleSize( delta, size.cx, imageSize.cx );

		CScopedGdiObj scopedBrush( pDC, ::GetSysColorBrush( COLOR_3DHIGHLIGHT ) );
		pDC->StretchBlt( pos.x + delta.cx, pos.y + delta.cy, size.cx, size.cy, &memDC, 0, 0, imageSize.cx, imageSize.cy, ROP_PSDPxax );

		// draw with shadow color
		scopedBrush.SelectObject( ::GetSysColorBrush( COLOR_3DSHADOW ) );
		pDC->StretchBlt( pos.x, pos.y, size.cx, size.cy, &memDC, 0, 0, imageSize.cx, imageSize.cy, ROP_PSDPxax );
		pDC->SetBkColor( oldBkColor );

		pDC->SetStretchBltMode( oldStretchBltMode );
		return true;
	}

	void DrawImageEffect( CDC* pDC, CImageList& rImageList, const CSize& imageSize, int index, const CPoint& pos, const CSize& size,
						  Effect effect, UINT style /*= ILD_TRANSPARENT*/, COLORREF blendToColor /*= ::GetSysColor( COLOR_BTNFACE )*/ )
	{
		switch ( effect )
		{
			case Normal:
				DrawImage( pDC, rImageList, imageSize, index, pos, size, style, blendToColor );
				break;
			case Disabled:
				DrawDisabledImage( pDC, rImageList, imageSize, index, pos, size, style, blendToColor );
				break;
			case Embossed:
				DrawEmbossedImage( pDC, rImageList, imageSize, index, pos, size, style, blendToColor );
				break;
			case Blend25:
				BlendImage( pDC, rImageList, imageSize, index, pos, size, blendToColor, 64 );
				break;
			case Blend50:
				BlendImage( pDC, rImageList, imageSize, index, pos, size, blendToColor, 128 );
				break;
			case Blend75:
				BlendImage( pDC, rImageList, imageSize, index, pos, size, blendToColor, 192 );
				break;
		}
	}


	bool MakeDisabledImageList( CImageList* pDestImageList, const CImageList& srcImageList, DisabledStyle style /*= gdi::Dis_FadeGray*/ )
	{
		ASSERT_PTR( pDestImageList );
		ASSERT_PTR( srcImageList.GetSafeHandle() );

		gdi::CImageInfo srcInfo( &srcImageList, 0 );

		CImageListStripProxy srcStripProxy( &srcImageList );
		CDibSection disabledDib;

		if ( !disabledDib.CreateDisabledEffectDIB32( &srcStripProxy, srcInfo.m_bitsPerPixel, style ) )		// create 32-bpp DIB section and draw onto it the entire SRC image strip
			return false;

		pDestImageList->DeleteImageList();
		disabledDib.CreateEmptyImageList( pDestImageList, srcInfo.m_imageSize, srcInfo.m_imageCount );		// compatible with source DIB

		if ( disabledDib.HasAlpha() )
			pDestImageList->Add( &disabledDib, (CBitmap*)nullptr );					// use alpha channel (no mask required)
		else if ( srcInfo.hbmMask != nullptr )
			pDestImageList->Add( &disabledDib, CBitmap::FromHandle( srcInfo.hbmMask ) );
		else
			pDestImageList->Add( &disabledDib, srcImageList.GetBkColor() );

		return true;
	}


	COLORREF MakeKeyColor( COLORREF color )
	{
		// shift the green component slightly so that original 'color' gets preserved in simulated background transparency
		CPixelBGR pixel( color );

		if ( pixel.m_green != 255 )
			++pixel.m_green;
		else
			--pixel.m_green;

		return pixel.GetColor();
	}
}
