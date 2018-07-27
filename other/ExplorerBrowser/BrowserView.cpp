// BrowserView.cpp : implementation of the CBrowserView class
//

#include "stdafx.h"
#include "BrowserView.h"
#include "BrowserDoc.h"
#include "Application.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


IMPLEMENT_DYNCREATE( CBrowserView, CView )

CBrowserView::CBrowserView( void )
	: CView()
	, m_showFrames( true )
	, m_dwAdviseCookie( 0 )
{
}

CBrowserView::~CBrowserView()
{
}

HRESULT CBrowserView::OnNavigationPending( PCIDLIST_ABSOLUTE pidlFolder )
{
	TRACE( _T("OnNavigationPending %s\n"), shell::GetFolderDisplayName( pidlFolder ).c_str() );
	return S_OK;
}

void CBrowserView::OnNavigationComplete( PCIDLIST_ABSOLUTE pidlFolder )
{
	TRACE( _T("OnNavigationComplete %s\n"), shell::GetFolderDisplayName( pidlFolder ).c_str() );
}

void CBrowserView::OnViewCreated( IShellView* pShellView )
{
	pShellView;
	TRACE( _T("OnViewCreated\n") );
}

void CBrowserView::OnNavigationFailed( PCIDLIST_ABSOLUTE pidlFolder )
{
	TRACE( _T("OnNavigationFailed %s\n"), shell::GetFolderDisplayName( pidlFolder ).c_str() );
}

FOLDERVIEWMODE CBrowserView::CmdToViewMode( UINT cmdId )
{
	switch ( cmdId )
	{
		case ID_VIEW_SMALLICON:		return FVM_SMALLICON;
		case ID_VIEW_LARGEICON:		return FVM_ICON;
		case ID_VIEW_LIST:			return FVM_LIST;
		case ID_VIEW_DETAILS:		return FVM_DETAILS;

		case ID_VIEW_TILES:			return FVM_TILE;
		case ID_VIEW_THUMBNAILS:	return FVM_THUMBNAIL;
		case ID_VIEW_THUMBSTRIP:	return FVM_THUMBSTRIP;
		default:
			ASSERT( false );
			return FVM_AUTO;
	}
}

BOOL CBrowserView::PreCreateWindow( CREATESTRUCT& cs )
{
	if ( !CView::PreCreateWindow( cs ) )
		return FALSE;

	cs.style |= WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
	return TRUE;
}


// message handlers

BEGIN_MESSAGE_MAP( CBrowserView, CView )
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_SIZE()
	ON_COMMAND( ID_FILE_RENAME, OnFileRename )

	ON_COMMAND( ID_VIEW_GOTOUSERPROFILE, OnBrowseToProfileFolder )
	ON_COMMAND_RANGE( ID_VIEW_SMALLICON, ID_VIEW_DETAILS, OnViewMode )
	ON_COMMAND_RANGE( ID_VIEW_TILES, ID_VIEW_THUMBSTRIP, OnViewMode )
	ON_UPDATE_COMMAND_UI_RANGE( ID_VIEW_SMALLICON, ID_VIEW_DETAILS, OnUpdateViewMode )
	ON_UPDATE_COMMAND_UI_RANGE( ID_VIEW_TILES, ID_VIEW_THUMBSTRIP, OnUpdateViewMode )
	ON_COMMAND( ID_VIEW_BACK, OnViewBack )
	ON_COMMAND( ID_VIEW_FORWARD, OnViewForward )
	ON_COMMAND( ID_VIEW_FRAMES, OnViewFrames )
	ON_UPDATE_COMMAND_UI( ID_VIEW_FRAMES, OnUpdateViewFrames )
	ON_COMMAND( ID_VIEW_SHOWSELECTION, OnViewShowselection )
END_MESSAGE_MAP()

int CBrowserView::OnCreate( CREATESTRUCT* pCreateStruct )
{
	if ( -1 == CView::OnCreate( pCreateStruct ) )
		return -1;

	m_pBrowser.reset( new shell::CExplorerBrowser );
	if ( m_pBrowser->Create( this, g_theApp.m_showFrames, GetDocument()->GetFilePaneViewMode() ) )
	{
		CComObject< CExplorerBrowserEvents >* pExplorerEvents;
		if ( SUCCEEDED( CComObject< CExplorerBrowserEvents >::CreateInstance( &pExplorerEvents ) ) )
		{
			pExplorerEvents->AddRef();

			pExplorerEvents->SetView( this );
			m_pBrowser->Get()->Advise( pExplorerEvents, &m_dwAdviseCookie );

			pExplorerEvents->Release();
			return 0;
		}
	}

	return -1;
}

void CBrowserView::OnDestroy( void )
{
	if ( m_pBrowser.get() != NULL )
	{
		CBrowserDoc* pDoc = GetDocument();

		pDoc->SetFilePaneViewMode( m_pBrowser->GetFilePaneViewMode() );
		pDoc->SetPathName( m_pBrowser->GetCurrentDirPath().c_str(), TRUE );

		std::vector< std::tstring > selPaths;
		m_pBrowser->QuerySelectedFiles( selPaths );
		pDoc->StoreSelItems( selPaths );
	}

	CView::OnDestroy();

	if ( m_dwAdviseCookie )
		m_pBrowser->Get()->Unadvise( m_dwAdviseCookie );

	m_pBrowser.reset();
}

void CBrowserView::OnInitialUpdate( void )
{
	CView::OnInitialUpdate();

	CBrowserDoc* pDoc = GetDocument();
	const CString& dirPath = pDoc->GetPathName();

	if ( fs::IsValidDirectory( dirPath ) )
		if ( m_pBrowser->NavigateTo( dirPath ) )			// navigate to document's directory
		{
			std::vector< std::tstring > selItems;
			if ( pDoc->QuerySelItems( selItems ) )
				m_pBrowser->SelectItems( selItems );

			return;
		}

	OnBrowseToProfileFolder();
}

void CBrowserView::OnSize( UINT sizeType, int cx, int cy )
{
	CView::OnSize( sizeType, cx, cy );

	if ( sizeType != SIZE_MINIMIZED )
		m_pBrowser->Get()->SetRect( NULL, CRect( 0, 0, cx, cy ) );
}

void CBrowserView::OnDraw( CDC* pDC )
{
	pDC;
}

void CBrowserView::OnFileRename( void )
{
	m_pBrowser->RenameFile();
}

void CBrowserView::OnBrowseToProfileFolder( void )
{
	HRESULT hr = S_OK;
	LPITEMIDLIST pidlBrowse = NULL;
	if ( SUCCEEDED( hr = ::SHGetFolderLocation( NULL, CSIDL_PROFILE, NULL, 0, &pidlBrowse ) ) )
	{
		if ( FAILED( hr = m_pBrowser->Get()->BrowseToIDList( pidlBrowse, 0 ) ) )
			TRACE( "BrowseToIDList Failed! hr = %d\n", hr );

		::ILFree( pidlBrowse );
	}
	else
		TRACE("SHGetFolderLocation Failed! hr = %d\n", hr);

	if ( FAILED( hr ) )
		AfxMessageBox( _T("Navigation failed!") );
}

void CBrowserView::OnViewMode( UINT cmdId )
{
	if ( !m_pBrowser->SetFilePaneViewMode( CmdToViewMode( cmdId ) ) )
		AfxMessageBox( _T("SetFolderSettings() failed!") );
}

void CBrowserView::OnUpdateViewMode( CCmdUI* pCmdUI )
{
	UINT folderFlags;
	FOLDERVIEWMODE selViewMode = m_pBrowser->GetFilePaneViewMode( &folderFlags );

	ui::SetRadio( pCmdUI, selViewMode == CmdToViewMode( pCmdUI->m_nID ) );
}

void CBrowserView::OnViewBack( void )
{
	m_pBrowser->NavigateBack();
}

void CBrowserView::OnViewForward( void )
{
	m_pBrowser->NavigateForward();
}

void CBrowserView::OnViewFrames( void )
{
	m_showFrames = !m_showFrames;
	m_pBrowser->Get()->SetOptions( m_showFrames ? EBO_SHOWFRAMES : EBO_NONE );
}

void CBrowserView::OnUpdateViewFrames( CCmdUI* pCmdUI )
{
	EXPLORER_BROWSER_OPTIONS options = EBO_NONE;
	m_pBrowser->Get()->GetOptions( &options );

	pCmdUI->SetCheck( HasFlag( options, EBO_SHOWFRAMES ) );
}

void CBrowserView::OnViewShowselection( void )
{
	std::vector< std::tstring > selPaths;
	m_pBrowser->QuerySelectedFiles( selPaths );

	CString message;
	message.Format( _T("Selected %d files:\n%s"), selPaths.size(), str::Join( selPaths, _T("\n") ).c_str() );

	AfxMessageBox( message );
}
