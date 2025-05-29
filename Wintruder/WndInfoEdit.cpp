
#include "pch.h"
#include "WndInfoEdit.h"
#include "AppService.h"
#include "wnd/WndImageRepository.h"
#include "wnd/WndUtils.h"
#include "utl/UI/WndUtils.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CWndInfoEdit::CWndInfoEdit( void )
	: CImageEdit()
	, m_hCurrentWnd( NULL )
{
	SetImageList( CWndImageRepository::Instance().GetImageList() );
}

CWndInfoEdit::~CWndInfoEdit()
{
}

void CWndInfoEdit::SetCurrentWnd( HWND hCurrentWnd )
{
	ASSERT_PTR( m_hWnd );
	m_hCurrentWnd = hCurrentWnd;

	std::tstring briefInfo;
	int imageIndex = Image_Transparent;

	if ( ui::IsValidWindow( m_hCurrentWnd ) )
	{
		briefInfo = wnd::FormatBriefWndInfo( m_hCurrentWnd );
		if ( NULL == wnd::GetWindowIcon( m_hCurrentWnd ) )			// otherwise the custom icon will be drawn over transparent
			imageIndex = CWndImageRepository::Instance().LookupImage( m_hCurrentWnd );
	}
	else
		briefInfo = _T("<n/a>");

	ui::SetWindowText( m_hWnd, briefInfo );
	SetImageIndex( imageIndex );
}

void CWndInfoEdit::DrawImage( CDC* pDC, const CRect& imageRect )
{
	CImageEdit::DrawImage( pDC, imageRect );

	if ( Image_Transparent == GetImageIndex() )
		if ( ui::IsValidWindow( m_hCurrentWnd ) )
			if ( HICON hIcon = wnd::GetWindowIcon( m_hCurrentWnd ) )
				::DrawIconEx( *pDC, imageRect.left, imageRect.top, hIcon, imageRect.Width(), imageRect.Height(), 0, NULL, DI_NORMAL | DI_COMPAT );
}
