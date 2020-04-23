
#include "stdafx.h"
#include "WicDibSection.h"
#include "GdiCoords.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CWicDibSection::~CWicDibSection()
{
}

void CWicDibSection::Clear( void )
{
	CWicBitmap::Clear();
	DeleteObject();
}

bool CWicDibSection::SetWicBitmap( IWICBitmapSource* pWicBitmap )
{
	return
		CWicBitmap::SetWicBitmap( pWicBitmap ) &&
		Attach( wic::CreateDibSection( pWicBitmap ) ) != FALSE;
}

HBITMAP CWicDibSection::CloneBitmap( void ) const
{
	ASSERT_PTR( GetSafeHandle() );

	return (HBITMAP)::CopyImage( GetSafeHandle(), IMAGE_BITMAP, GetBmpFmt().m_size.cx, GetBmpFmt().m_size.cy, LR_CREATEDIBSECTION );
}

CRect CWicDibSection::Draw( CDC* pDC, const CRect& boundsRect, ui::StretchMode stretchMode /*= ui::OriginalSize*/, CDC* pSrcDC /*= NULL*/, DWORD rop /*= SRCCOPY*/ )
{
	if ( !IsValid() )
		return boundsRect;

	const CRect bmpRect( CPoint( 0, 0 ), GetBmpFmt().m_size );
	CRect imageRect( CPoint( 0, 0 ), ui::StretchSize( boundsRect.Size(), bmpRect.Size(), stretchMode ) );
	ui::CenterRect( imageRect, boundsRect );

	BlitNatural( pDC, imageRect, bmpRect, pSrcDC, rop );
	return imageRect;
}

CRect CWicDibSection::DrawAtPos( CDC* pDC, const CPoint& pos, CDC* pSrcDC /*= NULL*/ )
{
	const CRect bmpRect( CPoint( 0, 0 ), GetBmpFmt().m_size );
	CRect imageRect( pos, bmpRect.Size() );

	BlitNatural( pDC, imageRect, bmpRect, pSrcDC, SRCCOPY );
	return imageRect;
}

bool CWicDibSection::BlitNatural( CDC* pDC, const CRect& destRect, const CRect& srcRect, CDC* pSrcDC /*= NULL*/, DWORD rop /*= SRCCOPY*/ ) const
{
	if ( !IsValid() )
		return false;

	CDC memDC;
	if ( NULL == pSrcDC && memDC.CreateCompatibleDC( pDC ) )
		pSrcDC = &memDC;

	if ( NULL == pSrcDC )
		return false;

	CScopedGdiObj scopedBitmap( pSrcDC, GetSafeHandle() );
	int oldStretchMode = pDC->SetStretchBltMode( COLORONCOLOR );
	bool succeeded = false;

	if ( GetBmpFmt().m_hasAlphaChannel && SRCCOPY == rop )		// alpha channel present?
	{	// draw transparently
		BLENDFUNCTION blendFunc;
		blendFunc.BlendOp = AC_SRC_OVER;						// use pre-multiplied alpha
		blendFunc.BlendFlags = 0;
		blendFunc.SourceConstantAlpha = 255;					// use per-pixel alpha values
		blendFunc.AlphaFormat = AC_SRC_ALPHA;

		succeeded = pDC->AlphaBlend( destRect.left, destRect.top, destRect.Width(), destRect.Height(), pSrcDC, srcRect.left, srcRect.top, srcRect.Width(), srcRect.Height(), blendFunc ) != FALSE;
	}

	if ( !succeeded )
		if ( destRect.Size() == srcRect.Size() )
			succeeded = pDC->BitBlt( destRect.left, destRect.top, destRect.Width(), destRect.Height(), pSrcDC, 0, 0, rop ) != FALSE;
		else
			succeeded = pDC->StretchBlt( destRect.left, destRect.top, destRect.Width(), destRect.Height(), pSrcDC, 0, 0, srcRect.Width(), srcRect.Height(), rop ) != FALSE;

	pDC->SetStretchBltMode( oldStretchMode );
	return succeeded;
}
