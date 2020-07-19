
#include "stdafx.h"
#include "AlbumChildFrame.h"
#include "MainFrame.h"
#include "AlbumImageView.h"
#include "AlbumThumbListView.h"
#include "Application.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


IMPLEMENT_DYNCREATE( CAlbumChildFrame, CChildFrame )


CAlbumChildFrame::CAlbumChildFrame( void )
	: CChildFrame()
	, m_pThumbsListView( NULL )
	, m_pAlbumImageView( NULL )
{
}

CAlbumChildFrame::~CAlbumChildFrame()
{
}

IImageView* CAlbumChildFrame::GetImageView( void ) const
{
	return m_pAlbumImageView;
}


// message handlers

BEGIN_MESSAGE_MAP( CAlbumChildFrame, CChildFrame )
	ON_WM_CREATE()
	ON_COMMAND_EX( ID_VIEW_ALBUMDIALOGBAR, OnBarCheck )
	ON_UPDATE_COMMAND_UI( ID_VIEW_ALBUMDIALOGBAR, OnUpdateBarCheck )		// CFrameWnd::OnUpdateControlBarMenu
END_MESSAGE_MAP()

int CAlbumChildFrame::OnCreate( CREATESTRUCT* pCS )
{
	if ( -1 == CChildFrame::OnCreate( pCS ) )
		return -1;

	// initialize album dialog-bar
	VERIFY( m_albumInfoBar.Create( this, IDD_ALBUMDIALOGBAR, CBRS_TOP | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_HIDE_INPLACE, ID_VIEW_ALBUMDIALOGBAR ) );
	//m_albumInfoBar.EnableWindow( FALSE );		// by default disable the navigation bar
	ShowControlBar( &m_albumInfoBar, FALSE, FALSE );
	return 0;
}

BOOL CAlbumChildFrame::OnCreateClient( CREATESTRUCT* pCS, CCreateContext* pContext )
{
	pCS;
	VERIFY( m_splitterWnd.CreateStatic( this, 1, 2, WS_CHILD | WS_VISIBLE, AFX_IDW_PANE_FIRST ) );

	CRect thumbViewRect = CAlbumThumbListView::GetListWindowRect();

	m_splitterWnd.SetColumnInfo( ThumbView, thumbViewRect.Width(), 0 );
	VERIFY( m_splitterWnd.CreateView( 0, ThumbView, RUNTIME_CLASS( CAlbumThumbListView ), CSize( thumbViewRect.Width(), 0 ), pContext ) );		// pass pContext so it won't send premature WM_INITIALUPDATE

	m_splitterWnd.SetColumnInfo( PictureView, 0, 30 );
	VERIFY( m_splitterWnd.CreateView( 0, PictureView, RUNTIME_CLASS( CAlbumImageView ), CSize( 0, 0 ), pContext ) );		// pass pContext so it won't send premature WM_INITIALUPDATE

	m_pThumbsListView = checked_static_cast< CAlbumThumbListView* >( m_splitterWnd.GetPane( 0, ThumbView ) );
	m_pAlbumImageView = checked_static_cast< CAlbumImageView* >( m_splitterWnd.GetPane( 0, PictureView ) );

	m_pThumbsListView->StorePeerView( m_pAlbumImageView );
	m_pAlbumImageView->StorePeerView( m_pThumbsListView, &m_albumInfoBar );
	return TRUE;
}

BOOL CAlbumChildFrame::OnBarCheck( UINT dlgBarId )
{
	ToggleFlag( GetAlbumImageView()->RefSlideData().m_viewFlags, af::ShowAlbumDialogBar );
	return CChildFrame::OnBarCheck( dlgBarId );	// toggle the visibility of the album dialog bar
}

void CAlbumChildFrame::OnUpdateBarCheck( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( !app::GetMainFrame()->IsFullScreen() );
	CFrameWnd::OnUpdateControlBarMenu( pCmdUI );
}
