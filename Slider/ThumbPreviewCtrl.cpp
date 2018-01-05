
#include "StdAfx.h"
#include "ThumbPreviewCtrl.h"
#include "AlbumThumbListView.h"
#include "Application.h"
#include "utl/Utilities.h"
#include "utl/Thumbnailer.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


void CThumbPreviewCtrl::SetImagePath( const fs::CFlexPath& imageFilePath, bool doRedraw /*= true*/ )
{
	m_imageFilePath = imageFilePath;
	if ( doRedraw )
		Invalidate();
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
	CPaintDC dc( this );
	CWicDibSection* pThumb = !m_imageFilePath.IsEmpty() ? pThumb = app::GetThumbnailer()->AcquireThumbnail( m_imageFilePath ) : NULL;

	CRect clientRect;
	GetClientRect( &clientRect );
	::FillRect( dc, clientRect, GetSysColorBrush( COLOR_BTNFACE ) );

	if ( pThumb != NULL )
		pThumb->Draw( &dc, clientRect, ui::ShrinkFit );
}
