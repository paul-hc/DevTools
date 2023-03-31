
#include "pch.h"
#include "DibDraw.h"
#include "DibSection.h"
#include "DibPixels.h"
#include "GdiCoords.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#include "Image_fwd.hxx"


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
		if ( !dib.CreateDIBSection( pixels, imageSize.cx, imageSize.cy, 32 ) )		// 32 bpp
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
							UINT style /*= ILD_TRANSPARENT*/, COLORREF blendToColor /*= GetSysColor( COLOR_BTNFACE )*/, BYTE toAlpha /*= 64*/ )
	{
		CDibSection dib;
		CDibPixels pixels;
		if ( dib.CreateDIBSection( pixels, imageSize.cx, imageSize.cy, 32 ) )		// 32 bpp
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
							UINT style /*= ILD_TRANSPARENT*/, COLORREF blendToColor /*= GetSysColor( COLOR_BTNFACE )*/, bool scaleHighlight /*= true*/ )
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

		CScopedGdiObj scopedBrush( pDC, GetSysColorBrush( COLOR_3DHIGHLIGHT ) );
		pDC->StretchBlt( pos.x + delta.cx, pos.y + delta.cy, size.cx, size.cy, &memDC, 0, 0, imageSize.cx, imageSize.cy, ROP_PSDPxax );

		// draw with shadow color
		scopedBrush.SelectObject( GetSysColorBrush( COLOR_3DSHADOW ) );
		pDC->StretchBlt( pos.x, pos.y, size.cx, size.cy, &memDC, 0, 0, imageSize.cx, imageSize.cy, ROP_PSDPxax );
		pDC->SetBkColor( oldBkColor );

		pDC->SetStretchBltMode( oldStretchBltMode );
		return true;
	}

	void DrawImageEffect( CDC* pDC, CImageList& rImageList, const CSize& imageSize, int index, const CPoint& pos, const CSize& size,
						  Effect effect, UINT style /*= ILD_TRANSPARENT*/, COLORREF blendToColor /*= GetSysColor( COLOR_BTNFACE )*/ )
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


	bool MakeDisabledImageList( CImageList& rDestImageList, const CImageList& srcImageList, DisabledStyle style /*= gdi::DisabledBlendColor*/,
								COLORREF blendToColor /*= GetSysColor( COLOR_BTNFACE )*/, BYTE toAlpha /*= 128*/ )
	{
		ASSERT_PTR( srcImageList.GetSafeHandle() );
		ASSERT( gdi::HasAlphaTransparency( srcImageList ) );		// not implemented for other DIBs

		CImageInfo info( &srcImageList, 0 );
		CDibPixels srcPixels( info.hbmImage );

		CDibSection dib;
		CDibPixels pixels( &dib );
		if ( !dib.CreateDIBSection( pixels, info.m_imageSize.cx * info.m_imageCount, info.m_imageSize.cy, srcPixels.GetBitsPerPixel() ) )
			return false;

		// src is vertical and dest is horizontal, so copy pixels image by image
		for ( UINT i = 0; i != info.m_imageCount; ++i )
		{
			CRect srcRect = info.MapSrcDibRect( i );
			CRect destRect = info.GetDestRect( i );
			pixels.CopyRect<CPixelBGRA, CPixelBGRA>( srcPixels, destRect, srcRect.TopLeft() );
		}

		switch ( style )
		{
			default: ASSERT( false );
			case DisabledGrayScale:		pixels.ApplyGrayScale(); break;
			case DisabledGrayOut:		pixels.ApplyDisabledGrayOut( blendToColor, toAlpha ); break;
			case DisabledEffect:		pixels.ApplyDisabledEffect( blendToColor, toAlpha ); break;
			case DisabledBlendColor:	pixels.ApplyBlendColor( blendToColor, toAlpha ); break;
		}

		rDestImageList.DeleteImageList();
		dib.CreateEmptyImageList( rDestImageList, info.m_imageSize, info.m_imageCount );		// compatible with source DIB
		if ( dib.HasAlpha() )
			rDestImageList.Add( &dib, (CBitmap*)nullptr );					// use alpha channel (no mask required)
		else if ( info.hbmMask != nullptr )
			rDestImageList.Add( &dib, CBitmap::FromHandle( info.hbmMask ) );
		else
			rDestImageList.Add( &dib, srcImageList.GetBkColor() );

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


	// CImageInfo implementation

	CImageInfo::CImageInfo( const CImageList* pImageList, int index )
		: m_pImageList( pImageList )
		, m_imageCount( m_pImageList->GetImageCount() )
	{
		VERIFY( m_pImageList->GetImageInfo( index, this ) );
		m_imageSize = ( (const CRect&)rcImage ).Size();
		m_srcDibSize = gdi::GetBitmapSize( hbmImage != nullptr ? hbmImage : hbmMask );
	}

	CRect CImageInfo::MapSrcDibRect( int index )
	{
		int extraHeight = m_srcDibSize.cy - m_imageSize.cy * m_imageCount;
		GetImageAt( m_imageCount - 1 - index );			// mirror the index (the DIB is bottom-up)
		CRect srcRect = rcImage;
		srcRect.OffsetRect( 0, extraHeight );			// offset up by empty extra space
		return srcRect;
	}
}
