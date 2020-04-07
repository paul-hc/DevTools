
#include "stdafx.h"
#include "ThumbPreviewCtrl.h"
#include "utl/UI/Utilities.h"
#include "utl/UI/Thumbnailer.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


void CThumbPreviewCtrl::SetImagePath( const fs::CFlexPath& imageFilePath, bool doRedraw /*= true*/ )
{
	m_imageFilePath = imageFilePath;
	if ( doRedraw )
		Invalidate();
}

void CThumbPreviewCtrl::DrawThumbnail( CDC* pDC, const CRect& clientRect, CWicDibSection* pThumb )
{
	ASSERT_PTR( pThumb );

	if ( !DrawDualThumbnailFlow( pDC, clientRect, pThumb ) )
		pThumb->Draw( pDC, clientRect, ui::ShrinkFit );		// standard drawing centered
}

bool CThumbPreviewCtrl::DrawDualThumbnailFlow( CDC* pDC, const CRect& clientRect, CWicDibSection* pThumb )
{
	const CSize& thumbSize = pThumb->GetBmpFmt().m_size;
	CSize halfDisplaySize = clientRect.Size();
	enum DisplayFlow { LeftToRight, TopToBottom } displayFlow = ui::GetAspectRatio( halfDisplaySize ) >= 1.0 ? LeftToRight : TopToBottom;

	if ( LeftToRight == displayFlow )
		halfDisplaySize.cx /= 2;
	else
		halfDisplaySize.cy /= 2;

	if ( ui::FitsInside( halfDisplaySize, thumbSize ) )
	{
		CSize scaledImageSize = ui::StretchToFit( halfDisplaySize, thumbSize );
		int zoomMagnifyFactor = scaledImageSize.cx / thumbSize.cx;

		if ( zoomMagnifyFactor >= 2 )		// at least an integral doubling of the large thumb?
		{
			CRect normalRect = clientRect, zoomedRect = clientRect;

			if ( LeftToRight == displayFlow )
			{
				normalRect.right -= halfDisplaySize.cx;
				zoomedRect.left += halfDisplaySize.cx;
			}
			else
			{
				normalRect.bottom -= halfDisplaySize.cy;
				zoomedRect.top += halfDisplaySize.cy;
			}

			pThumb->Draw( pDC, normalRect, ui::ShrinkFit );			// normal drawing (not zoomed)

			// magnified drawing (zoomed by integral factor)
			::FillRect( pDC->m_hDC, zoomedRect, GetSysColorBrush( COLOR_SCROLLBAR ) );		// slightly darker background

			CRect zoomedImageRect( CPoint( 0, 0 ), ui::ScaleSize( thumbSize, zoomMagnifyFactor * 100 ) );
			ui::CenterRect( zoomedImageRect, zoomedRect );

			pThumb->Draw( pDC, zoomedImageRect, ui::StretchFit );
			return true;
		}
	}
	return false;
}


// message handlers

BEGIN_MESSAGE_MAP( CThumbPreviewCtrl, CStatic )
	ON_WM_ERASEBKGND()
	ON_WM_PAINT()
END_MESSAGE_MAP()

BOOL CThumbPreviewCtrl::OnEraseBkgnd( CDC* pDC )
{
	pDC;
	return TRUE;
}

void CThumbPreviewCtrl::OnPaint( void )
{
	CWicDibSection* pThumb = NULL;
	if ( !m_imageFilePath.IsEmpty() )
		pThumb = m_pThumbnailer->AcquireThumbnail( m_imageFilePath );

	CPaintDC dc( this );
	CRect clientRect;
	GetClientRect( &clientRect );
	::FillRect( dc, clientRect, GetSysColorBrush( COLOR_BTNFACE ) );

	if ( pThumb != NULL )
		DrawThumbnail( &dc, clientRect, pThumb );
}
