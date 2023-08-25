
#include "pch.h"
#include "AlbumChildFrame.h"
#include "AlbumImageView.h"
#include "AlbumThumbListView.h"
#include "resource.h"
#include "utl/UI/PostCall.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


IMPLEMENT_DYNCREATE( CAlbumChildFrame, CChildFrame )

CAlbumChildFrame::CAlbumChildFrame( void )
	: CChildFrame()
	, m_pThumbsListView( nullptr )
	, m_pAlbumImageView( nullptr )
{
	GetDockingManager()->DisableRestoreDockState( TRUE );		// to disable loading of docking layout from the Registry
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
	ON_COMMAND( ID_VIEW_ALBUMDIALOGBAR, OnToggle_ViewAlbumPane )
	ON_UPDATE_COMMAND_UI( ID_VIEW_ALBUMDIALOGBAR, OnUpdate_ViewAlbumPane )		// CFrameWnd::OnUpdateControlBarMenu
END_MESSAGE_MAP()

int CAlbumChildFrame::OnCreate( CREATESTRUCT* pCS )
{
	if ( -1 == CChildFrame::OnCreate( pCS ) )
		return -1;

	// initialize album dialog-bar
	enum { PaneStyle = WS_VISIBLE | WS_CHILD | CBRS_TOP | CBRS_TOOLTIPS | CBRS_FLYBY };

	VERIFY( m_albumDlgPane.Create( _T(""), this, false, IDD_ALBUMDIALOGBAR, PaneStyle, ID_VIEW_ALBUMDIALOGBAR ) );

	m_albumDlgPane.DockToFrameWindow( CBRS_ALIGN_TOP );		// make the dialog pane stretchable horizontally
	//DockPane( &m_albumDlgPane );

	// note: it's too early to update pane visibility; we have to delay to after loading CSlideData data-memberby CAlbumDoc, in CAlbumImageView::UpdateChildBarsState()
	return 0;
}

BOOL CAlbumChildFrame::OnCreateClient( CREATESTRUCT* pCS, CCreateContext* pContext ) overrides(CChildFrame)
{
	pCS;
	VERIFY( m_splitterWnd.CreateStatic( this, 1, 2, WS_CHILD | WS_VISIBLE, AFX_IDW_PANE_FIRST ) );

	CRect thumbViewRect = CAlbumThumbListView::GetListWindowRect();

	m_splitterWnd.SetColumnInfo( ThumbView, thumbViewRect.Width(), 0 );
	VERIFY( m_splitterWnd.CreateView( 0, ThumbView, RUNTIME_CLASS( CAlbumThumbListView ), CSize( thumbViewRect.Width(), 0 ), pContext ) );		// pass pContext so it won't send premature WM_INITIALUPDATE

	m_splitterWnd.SetColumnInfo( PictureView, 0, 30 );
	VERIFY( m_splitterWnd.CreateView( 0, PictureView, RUNTIME_CLASS( CAlbumImageView ), CSize( 0, 0 ), pContext ) );		// pass pContext so it won't send premature WM_INITIALUPDATE

	m_pThumbsListView = checked_static_cast<CAlbumThumbListView*>( m_splitterWnd.GetPane( 0, ThumbView ) );
	m_pAlbumImageView = checked_static_cast<CAlbumImageView*>( m_splitterWnd.GetPane( 0, PictureView ) );
	StoreImageView( m_pAlbumImageView );

	m_pThumbsListView->StorePeerView( m_pAlbumImageView );
	m_pAlbumImageView->StorePeerView( m_pThumbsListView, &m_albumDlgPane );
	return TRUE;
}

void CAlbumChildFrame::OnToggle_ViewAlbumPane( void )
{	// toggle the visibility of the album dialog bar
	REQUIRE( (BOOL)GetAlbumImageView()->GetSlideData().HasShowFlag( af::ShowAlbumDialogBar ) == m_albumDlgPane.IsVisible() );	// consistent?

	GetAlbumImageView()->RefSlideData()->ToggleShowFlag( af::ShowAlbumDialogBar );		// toggle visibility flag
	bool show = GetAlbumImageView()->GetSlideData().HasShowFlag( af::ShowAlbumDialogBar );

	ShowPane( &m_albumDlgPane, show, false, false );
}

void CAlbumChildFrame::OnUpdate_ViewAlbumPane( CCmdUI* pCmdUI )
{
	pCmdUI->Enable();
	pCmdUI->SetCheck( m_albumDlgPane.IsVisible() );
}
