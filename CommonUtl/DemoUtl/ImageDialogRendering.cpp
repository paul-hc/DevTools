
#include "pch.h"
#include "ImageDialog.h"
#include "utl/ContainerOwnership.h"
#include "utl/ScopedValue.h"
#include "utl/UI/DibDraw.h"
#include "utl/UI/DibSection.h"
#include "utl/UI/DibPixels.h"
#include "utl/UI/ImagingGdiPlus.h"
#include "utl/UI/WndUtils.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#include "utl/UI/Image_fwd.hxx"


void CImageDialog::CreateEffectDibs( void )
{
	m_modeData[ m_sampleMode ]->Clear();
	m_pImageList.reset();
	if ( !IsValidImageMode() )
		return;

	CWaitCursor wait;
	CModeData* pModeData = m_modeData[ m_sampleMode ];
	CScopedFlag<int> scopedSkipCopyImage( &CDibSection::s_testFlags, m_convertFlags & CDibSection::ForceCvtEqualBpp );
	TBitsPerPixel bitsPerPixel = m_pDibSection->GetBitsPerPixel();
	COLORREF bkColor = GetBkColor();
	std::auto_ptr<CDibSection> pNewDib;

	pModeData->m_dibs.clear();			// allow push-backs
	switch ( m_sampleMode )
	{
		case ConvertImage:				// enum EffectDib {  };
		{
			CScopedFlag<int> scopedCopyPixels( &CDibSection::s_testFlags, m_convertFlags & CDibSection::ForceCvtCopyPixels );
			static const WORD bpp[] = { 1, 4, 8, 16, 24, 32 };
			ASSERT( COUNT_OF( bpp ) == pModeData->GetZoneCount() - 1 );
			for ( unsigned int i = 0; i != COUNT_OF( bpp ); ++i )
			{
				std::auto_ptr<CDibSection> pCvtDib( new CDibSection() );
				if ( !pCvtDib->Convert( *m_pDibSection, bpp[ i ] ) )
					pCvtDib.reset();

				pModeData->PushDib( pCvtDib );
			}
			break;
		}
		case ContrastImage:					// enum EffectDib { GrayScale };
			if ( utl::ResetPtr( pNewDib, CloneSourceDib() ) )
			{
				CDibPixels grayedPixels( pNewDib.get() );
				if ( !grayedPixels.ForEach( func::AdjustContrast( m_contrastPct ) ) )			// use bkColor as transparent for bpp<32
					utl::ResetPtr( pNewDib );
			}
			pModeData->PushDib( pNewDib );
			break;
		case GrayScale:					// enum EffectDib { GrayScale };
			if ( utl::ResetPtr( pNewDib, CloneSourceDib() ) )
			{
				CDibPixels grayedPixels( pNewDib.get() );
				if ( !grayedPixels.ApplyGrayScale( bkColor ) )			// use bkColor as transparent for bpp<32
					utl::ResetPtr( pNewDib );
			}
			pModeData->PushDib( pNewDib );
			break;
		case AlphaBlend:
			if ( utl::ResetPtr( pNewDib, CloneSourceDib() ) )
			{
				CDibPixels multipliedPixels( pNewDib.get() );
				if ( !multipliedPixels.ApplyAlphaBlend( m_pixelAlpha, bkColor ) )
					utl::ResetPtr( pNewDib );
			}
			pModeData->PushDib( pNewDib );

			pNewDib.reset();
			if ( bitsPerPixel >= 24 )
				if ( utl::ResetPtr( pNewDib, CloneSourceDib() ) )
				{
					CDibPixels gradPixels( pNewDib.get() );
					utl::HorizGradientAlphaBlend( gradPixels, bkColor );
				}
			pModeData->PushDib( pNewDib );
			break;
		case BlendColor:				// enum EffectDib { Blended };
			if ( utl::ResetPtr( pNewDib, CloneSourceDib() ) )
			{
				CDibPixels blendedPixels( pNewDib.get() );
				if ( !blendedPixels.ApplyBlendColor( bkColor, m_blendColor ) )
					utl::ResetPtr( pNewDib );
			}
			pModeData->PushDib( pNewDib );
			break;
		case Disabled:			// enum EffectDib { GrayScale, Blended, Disabled, DisabledGrayOut };
			if ( utl::ResetPtr( pNewDib, CloneSourceDib() ) )
			{
				CDibPixels grayedPixels( pNewDib.get() );
				if ( !grayedPixels.ApplyGrayScale( bkColor ) )
					utl::ResetPtr( pNewDib );
			}
			pModeData->PushDib( pNewDib );

			if ( utl::ResetPtr( pNewDib, CloneSourceDib() ) )
			{
				CDibPixels blendedPixels( pNewDib.get() );
				if ( !blendedPixels.ApplyBlendColor( bkColor, m_blendColor ) )
					utl::ResetPtr( pNewDib );
			}
			pModeData->PushDib( pNewDib );

			if ( utl::ResetPtr( pNewDib, CloneSourceDib() ) )
			{
				CDibPixels disabledPixels( pNewDib.get() );
				if ( !disabledPixels.ApplyDisabledEffect( bkColor, m_disabledAlpha ) )
					utl::ResetPtr( pNewDib );
			}
			pModeData->PushDib( pNewDib );

			if ( utl::ResetPtr( pNewDib, CloneSourceDib() ) )
			{
				CDibPixels disabledPixels( pNewDib.get() );
				if ( !disabledPixels.ApplyDisabledGrayOut( bkColor, m_disabledAlpha ) )
					utl::ResetPtr( pNewDib );
			}
			pModeData->PushDib( pNewDib );

			if ( utl::ResetPtr( pNewDib, CloneSourceDib() ) )
			{
				CDibPixels disabledPixels( pNewDib.get() );
				if ( !disabledPixels.ApplyDisableFadeGray( m_pDibSection->GetBitsPerPixel(), m_disabledAlpha ) )
					utl::ResetPtr( pNewDib );
			}
			pModeData->PushDib( pNewDib );
			break;
		case ImageList:
			if ( utl::ResetPtr( pNewDib, CloneSourceDib() ) )	// work on a copy since MakeImageList() blackens the transparent background for bpp<32
			{
				int imageCount = 1;
				CSize imageSize = m_pDibSection->GetSize();
				if ( 0 == ( imageSize.cx % imageSize.cy ) )		// width exact multiple of height?
				{
					imageCount = imageSize.cx / imageSize.cy;
					imageSize.cx /= imageCount;
				}
				else											// 16x15 case?
				{
					imageCount = imageSize.cx / imageSize.cy;
					if ( imageCount != 0 && imageSize.cx == imageSize.cx / imageCount * imageCount )
						imageSize.cx /= imageCount;
					else
						imageCount = 1;
				}

				m_pImageList.reset( new CImageList() );
				pNewDib->MakeImageList( m_pImageList.get(), imageCount );
			}
			break;
	}
}

void CImageDialog::Render_AllImages( CDC* pDC, CMultiZoneIterator* pMultiZone )
{
	BlendDib( m_pDibSection.get(), pDC, pMultiZone->GetNextZone() );

	const CModeData* pModeData = m_modeData[ m_sampleMode ];
	for ( std::vector<CDibSection*>::const_iterator itDib = pModeData->m_dibs.begin(); itDib != pModeData->m_dibs.end(); ++itDib )
		BlendDib( *itDib, pDC, pMultiZone->GetNextZone() );
}

void CImageDialog::Render_AlphaBlend( CDC* pDC, CMultiZoneIterator* pMultiZone )
{
	enum EffectDib { PixelMultiplied, PixelGradient };

	BlendDib( m_pDibSection.get(), pDC, pMultiZone->GetNextZone() );					// original image
	BlendDib( m_pDibSection.get(), pDC, pMultiZone->GetNextZone(), m_sourceAlpha );		// alpha-blend with source constant alpha
	BlendDib( GetSafeDibAt( PixelMultiplied ), pDC, pMultiZone->GetNextZone() );		// pixel alpha-multiplied
	BlendDib( GetSafeDibAt( PixelGradient ), pDC, pMultiZone->GetNextZone() );			// pixel gradient alpha-multiplied
}

void CImageDialog::Render_ImageList( CDC* pDC, CMultiZoneIterator* pMultiZone )
{
	enum Effect { Original, Disabled, Embossed, Blended25, Blended50, Blended75 };

	COLORREF blendToColor = ::GetSysColor( COLOR_BTNFACE );			// GetBkColor()

	DrawImageListEffect( pDC, gdi::Normal, blendToColor, pMultiZone->GetNextZone() );
	DrawImageListEffect( pDC, gdi::Disabled, blendToColor, pMultiZone->GetNextZone() );
	DrawImageListEffect( pDC, gdi::Embossed, blendToColor, pMultiZone->GetNextZone() );
	DrawImageListEffect( pDC, gdi::Blend25, blendToColor, pMultiZone->GetNextZone() );
	DrawImageListEffect( pDC, gdi::Blend50, blendToColor, pMultiZone->GetNextZone() );
	DrawImageListEffect( pDC, gdi::Blend75, blendToColor, pMultiZone->GetNextZone() );
}

void CImageDialog::DrawImageListEffect( CDC* pDC, gdi::Effect effect, COLORREF blendToColor, const CRect& boundsRect )
{
	int imageCount = m_pImageList->GetImageCount();
	CSize imageSize = gdi::GetImageIconSize( *m_pImageList );
	CSize scaledImageSize = ui::ScaleSize( imageSize, boundsRect.Height(), imageSize.cy );

	CMultiZoneIterator imageZone( scaledImageSize, IL_SpacingX, imageCount, CMultiZone::HorizStacked, boundsRect );

	for ( int i = 0; i != imageCount; ++i )
	{
		CRect imageRect = imageZone.GetNextZone();
		gdi::DrawImageEffect( pDC, *m_pImageList, imageSize, i, imageRect.TopLeft(), imageRect.Size(), effect, ILD_TRANSPARENT, blendToColor );

		if ( HasFlag( m_showFlags, ShowGuides ) )
			m_sampleView.DrawContentFrame( pDC, imageRect, color::Red, 64 );
	}
}

/********************************		ms-help://MS.VSCC.v90/MS.MSDNQTR.v90.en/gdi/bitmaps_2b00.htm
	Alpha Blending a Bitmap

	The following code sample divides a window into three horizontal areas.
	Then it draws an alpha-blended bitmap in each of the window areas as follows:

	In the top area, constant alpha = 50% but there is no source alpha.
	In the middle area, constant alpha = 100% (disabled) and source alpha is 0 (transparent) in the middle of the bitmap and 0xff (opaque) elsewhere.
	In the bottom area, constant alpha = 75% and source alpha changes.
*/
bool CImageDialog::Render_RectsAlphaBlend( CDC* pDC, const CRect& clientRect )
{
	ASSERT_PTR( pDC );

	CSize areaSize = clientRect.Size();
	areaSize.cy /= 3;	// divide the window into 3 horizontal areas
	if ( 0 == areaSize.cx || 0 == areaSize.cy )
		return false;

	CSize bitmapSize( areaSize.cx - ( areaSize.cx / 5 ) * 2, areaSize.cy - ( areaSize.cy / 5 ) * 2 );

	CDibSection dibSection;
	CDibPixels dibPixels;

	if ( !dibSection.CreateDIBSection( &dibPixels, bitmapSize.cx, bitmapSize.cy, ILC_COLOR32 ) )		// create the DIB section and select the bitmap into the dc
		return false;

	CScopedBitmapMemDC scopedBitmap( &dibSection, pDC );
	CDC* pMemDC = dibSection.GetBitmapMemDC();			// the source DC for AlphaBlend
	ASSERT_PTR( pMemDC );

	// TOP AREA: constant alpha = 50%, but no source alpha
	// set all pixels to blue and set source alpha to zero

	typedef CPixelBGRA* iterator;
	for ( iterator pPixel = dibPixels.Begin<CPixelBGRA>(), pPixelEnd = dibPixels.End<CPixelBGRA>(); pPixel != pPixelEnd; ++pPixel )
	{
		pPixel->m_alpha = 0;
		pPixel->m_red = 0;
		pPixel->m_green = 0;
		pPixel->m_blue = 0xFF;
	}

	// highlight a magenta 10x5 square in the origin corner
	for ( int y = 0; y != ( bitmapSize.cy / 5 ); ++y )
		for ( int x = 0; x != ( bitmapSize.cx / 10 ); ++x )
			dibPixels.GetPixel<CPixelBGRA>( x, y ).m_red = 0xFF;

	BLENDFUNCTION blendFunc;				// structure for alpha blending
	blendFunc.BlendOp = AC_SRC_OVER;
	blendFunc.BlendFlags = 0;
	blendFunc.SourceConstantAlpha = 0x7F;	// half of 0xFF = 50% transparency
	blendFunc.AlphaFormat = 0;				// ignore source alpha channel

	if ( !pDC->AlphaBlend( areaSize.cx / 5, areaSize.cy / 5, bitmapSize.cx, bitmapSize.cy,
						   pMemDC, 0, 0, bitmapSize.cx, bitmapSize.cy, blendFunc ) )
		return false;

	// MIDDLE AREA: constant alpha = 100% (disabled), source
	// alpha is 0 in middle of bitmap and opaque in rest of bitmap
	for ( int y = 0; y != bitmapSize.cy; ++y )
		for ( int x = 0; x != bitmapSize.cx; ++x )
		{
			CPixelBGRA& rPixel = dibPixels.GetPixel<CPixelBGRA>( x, y );

			if ( ( x > (int)( bitmapSize.cx / 5 ) ) && ( x < ( bitmapSize.cx - bitmapSize.cx / 5 ) ) &&
				 ( y > (int)( bitmapSize.cy / 5 ) ) && ( y < ( bitmapSize.cy - bitmapSize.cy / 5 ) ) )
				// in middle of bitmap: source alpha = 0 (transparent).
				// This means multiply each color component by 0x00.
				// Thus, after AlphaBlend, we have a, 0x00 * r, 0x00 * g, and 0x00 * b (which is 0x00000000)
				// for now, set all pixels to red
				rPixel = CPixelBGRA( color::Red, 0 );
			else
				// in the rest of bitmap, source alpha = 0xff (opaque) and set all pixels to blue
				rPixel = CPixelBGRA( color::Blue, 0xFF );
		}

	blendFunc.BlendOp = AC_SRC_OVER;
	blendFunc.BlendFlags = 0;
	blendFunc.SourceConstantAlpha = 0xFF;  // opaque (disable constant alpha)
	blendFunc.AlphaFormat = AC_SRC_ALPHA;  // use source alpha

	if ( !pDC->AlphaBlend( areaSize.cx / 5, areaSize.cy / 5 + areaSize.cy, bitmapSize.cx, bitmapSize.cy,
						   pMemDC, 0, 0, bitmapSize.cx, bitmapSize.cy, blendFunc ) )
		return false;

	// BOTTOM AREA: use constant alpha = 75% and a changing source alpha.
	// Create a gradient effect using source alpha, and then fade it even more with constant alpha.
	BYTE red = 0x00;
	BYTE green = 0x00;
	BYTE blue = 0xFF;

	for ( int y = 0; y != bitmapSize.cy; ++y )
		for ( int x = 0; x != bitmapSize.cx; ++x )
		{
			// for a simple gradient, base the alpha value on the x value of the pixel
			BYTE alpha = (BYTE)( (float)x / (float)bitmapSize.cx * 255 );
			float alphaFactor = (float)alpha / (float)0xFF;		// calculate the factor by which we multiply each component (for premultiply)

			// multiply each pixel by alphaFactor, so each component is less than or equal to the alpha value
			CPixelBGRA& rPixel = dibPixels.GetPixel<CPixelBGRA>( x, y );
			rPixel = CPixelBGRA( (BYTE)( red * alphaFactor ), (BYTE)( green * alphaFactor ), (BYTE)( blue * alphaFactor ), alpha );
		}

	blendFunc.BlendOp = AC_SRC_OVER;
	blendFunc.BlendFlags = 0;
	blendFunc.SourceConstantAlpha = 0xBF;   // use constant alpha, with 75% opaqueness
	blendFunc.AlphaFormat = AC_SRC_ALPHA;   // use source alpha

	if ( !pDC->AlphaBlend( areaSize.cx / 5, areaSize.cy / 5 + 2 * areaSize.cy, bitmapSize.cx, bitmapSize.cy,
						   pMemDC, 0, 0, bitmapSize.cx, bitmapSize.cy, blendFunc ) )
		return false;

	return true;
}

CDibSection* CImageDialog::CloneSourceDib( void ) const
{
	std::auto_ptr<CDibSection> pNewDib( new CDibSection() );

	if ( !pNewDib->Copy( m_pDibSection.get() ) )
		pNewDib.reset();

	return pNewDib.release();
}

CDibSection* CImageDialog::GetSafeDibAt( size_t dibPos ) const
{
	const CModeData* pModeData = m_modeData[ m_sampleMode ];
	ASSERT_PTR( pModeData );
	return dibPos < pModeData->m_dibs.size() ? pModeData->m_dibs[ dibPos ] : nullptr;
}

bool CImageDialog::BlendDib( CDibSection* pDib, CDC* pDC, const CRect& rect, BYTE srcAlpha /*= 255*/ )
{
	CDibSection::DrawResult result = CDibSection::Error;
	if ( pDib != nullptr && pDib->IsValid() )
	{
		result = pDib->DrawTransparent( pDC, rect, srcAlpha );
		if ( CDibSection::Error == result )
			if ( pDib->Blit( pDC, rect ) )
				result = CDibSection::Blitted;
	}

	switch ( result )
	{
		default: ASSERT( false );
		case CDibSection::Error:
			m_sampleView.DrawDiagonalCross( pDC, rect, color::Red, 255 );					// red for error
			return false;
		case CDibSection::AlphaBlended:
			break;
		case CDibSection::TranspDrawn:
			m_sampleView.DrawCross( pDC, rect, color::Magenta, 50 );
			break;
		case CDibSection::Blitted:
			if ( 255 == srcAlpha )
				m_sampleView.DrawDiagonalCross( pDC, rect, color::AzureBlue, m_statusAlpha );						// blitted with no transparency
			else
				m_sampleView.DrawDiagonalCross( pDC, rect, color::Red, std::max<BYTE>( m_statusAlpha, 192 ) );	// custom alpha-blend failed
			break;
	}
	return true;
}
