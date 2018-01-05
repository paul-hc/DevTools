
#include "stdafx.h"
#include "WicDibSection.h"
#include "Utilities.h"

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

	CDC memDC;
	if ( NULL == pSrcDC && memDC.CreateCompatibleDC( pDC ) )
		pSrcDC = &memDC;

	const CSize& bmpSize = GetBmpFmt().m_size;
	CRect imageRect( CPoint( 0, 0 ), ui::StretchSize( boundsRect.Size(), bmpSize, stretchMode ) );
	ui::CenterRect( imageRect, boundsRect );

	if ( pSrcDC != NULL )
	{
		CScopedGdiObj scopedBitmap( pSrcDC, GetSafeHandle() );

		if ( bmpSize == imageRect.Size() )
			pDC->BitBlt( imageRect.left, imageRect.top, imageRect.Width(), imageRect.Height(), pSrcDC, 0, 0, rop );
		else
		{
			int oldStretchMode = pDC->SetStretchBltMode( COLORONCOLOR );
			pDC->StretchBlt( imageRect.left, imageRect.top, imageRect.Width(), imageRect.Height(), pSrcDC, 0, 0, bmpSize.cx, bmpSize.cy, rop );
			pDC->SetStretchBltMode( oldStretchMode );
		}
	}
	return imageRect;
}

CRect CWicDibSection::DrawAtPos( CDC* pDC, const CPoint& pos, CDC* pSrcDC /*= NULL*/ )
{
	CDC memDC;
	if ( NULL == pSrcDC && memDC.CreateCompatibleDC( pDC ) )
		pSrcDC = &memDC;

	CRect imageRect( pos, GetBmpFmt().m_size );
	if ( pSrcDC != NULL )
	{
		CScopedGdiObj scopedBitmap( pSrcDC, GetSafeHandle() );
		pDC->BitBlt( imageRect.left, imageRect.top, imageRect.Width(), imageRect.Height(), pSrcDC, 0, 0, SRCCOPY );
	}
	return imageRect;
}
