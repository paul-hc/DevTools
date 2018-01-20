// BrowserView.cpp : implementation of the CBrowserView class
//

#include "stdafx.h"
#include "BrowserView.h"
#include "BrowserDoc.h"
#include "Utilities.h"
#include "Application.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace shell
{
	bool NavigateTo( const TCHAR path[], IExplorerBrowser* pExplorerBrowser )
	{
		ASSERT_PTR( pExplorerBrowser );
		LPITEMIDLIST pidlBrowse;
		if ( SUCCEEDED( ::SHParseDisplayName( path, NULL, &pidlBrowse, 0, NULL ) ) )
			if ( SUCCEEDED( pExplorerBrowser->BrowseToIDList( pidlBrowse, 0 ) ) )
			{
				::ILFree( pidlBrowse );
				return true;
			}

		return false;
	}

	void QuerySelectedFiles( std::vector< std::tstring >& rSelPaths, IExplorerBrowser* pExplorerBrowser )
	{
		ASSERT_PTR( pExplorerBrowser );
		CComPtr< IShellView > pShellView;
		if ( SUCCEEDED( pExplorerBrowser->GetCurrentView( IID_PPV_ARGS( &pShellView ) ) ) )
		{
			CComPtr< IDataObject > pDataObject;
			if ( SUCCEEDED( pShellView->GetItemObject( SVGIO_SELECTION, IID_PPV_ARGS( &pDataObject ) ) ) )
			{
				//Code adapted from http://www.codeproject.com/shell/shellextguide1.asp
				FORMATETC fmt = { CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
				STGMEDIUM stg;
				stg.tymed =  TYMED_HGLOBAL;

				if ( SUCCEEDED( pDataObject->GetData( &fmt, &stg ) ) )
				{
					HDROP hDrop = (HDROP)::GlobalLock( stg.hGlobal );

					for ( UINT i = 0, fileCount = DragQueryFile( hDrop, 0xFFFFFFFF, NULL, 0 ); i < fileCount; ++i )
					{
						TCHAR path[ _MAX_PATH ] = { '0' };
						DragQueryFile( hDrop, i, path, MAX_PATH );

						if ( path[ 0 ] != 0 )
							rSelPaths.push_back( path );
					}

					::GlobalUnlock( stg.hGlobal );
					::ReleaseStgMedium( &stg );
				}
			}
		}
	}

	FOLDERVIEWMODE GetFolderViewMode( IExplorerBrowser* pExplorerBrowser, UINT* pFolderFlags /*= NULL*/ )
	{
		ASSERT_PTR( pExplorerBrowser );

		CComPtr< IShellView > pShellView;
		if ( SUCCEEDED( pExplorerBrowser->GetCurrentView( IID_PPV_ARGS( &pShellView ) ) ) )
		{
			FOLDERSETTINGS settings;
			if ( SUCCEEDED( pShellView->GetCurrentInfo( &settings ) ) )
			{
				if ( pFolderFlags != NULL )
					*pFolderFlags = settings.fFlags;

				return static_cast< FOLDERVIEWMODE >( settings.ViewMode );
			}
		}
		return FVM_AUTO;
	}

	std::tstring GetFolderDisplayName( PCIDLIST_ABSOLUTE pidlFolder )
	{
		std::tstring displayName;

		CComPtr< IShellFolder > pDesktopFolder;
		if ( SUCCEEDED( ::SHGetDesktopFolder( &pDesktopFolder ) ) )
		{
			STRRET sDisplayName;
			if ( SUCCEEDED( pDesktopFolder->GetDisplayNameOf( pidlFolder, SHGDN_FORPARSING, &sDisplayName ) ) )
			{
				LPTSTR szDisplayName = NULL;
				StrRetToStr( &sDisplayName, pidlFolder, &szDisplayName );
				displayName = szDisplayName;
				CoTaskMemFree( szDisplayName );
			}
		}

		return displayName;
	}

	FOLDERVIEWMODE CmdToViewMode( UINT cmdId )
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
}


namespace reg
{
	static const TCHAR section_view[] = _T("View");
	static const TCHAR entry_viewMode[] = _T("ViewMode");
}


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
	TRACE( "OnNavigationPending %S\n", shell::GetFolderDisplayName( pidlFolder ).c_str() );
	return S_OK;
}

void CBrowserView::OnNavigationComplete( PCIDLIST_ABSOLUTE pidlFolder )
{
	TRACE( "OnNavigationComplete %S\n", shell::GetFolderDisplayName( pidlFolder ).c_str() );
}

void CBrowserView::OnViewCreated( IShellView *psv )
{
	TRACE( "OnViewCreated\n" );
}

void CBrowserView::OnNavigationFailed( PCIDLIST_ABSOLUTE pidlFolder )
{
	TRACE( "OnNavigationFailed %S\n", shell::GetFolderDisplayName( pidlFolder ).c_str() );
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
	ON_COMMAND( ID_VIEW_GOTOUSERPROFILE, OnBrowseToProfileFolder )
	ON_COMMAND_RANGE( ID_VIEW_SMALLICON, ID_VIEW_DETAILS, OnViewMode )
	ON_COMMAND_RANGE( ID_VIEW_TILES, ID_VIEW_THUMBSTRIP, OnViewMode )
	ON_UPDATE_COMMAND_UI_RANGE( ID_VIEW_SMALLICON, ID_VIEW_DETAILS, OnUpdateViewMode )
	ON_UPDATE_COMMAND_UI_RANGE( ID_VIEW_TILES, ID_VIEW_THUMBSTRIP, OnUpdateViewMode )
	ON_COMMAND( ID_VIEW_BACK, OnViewBack )
	ON_COMMAND( ID_VIEW_FORWARD, OnViewForward )
	ON_COMMAND( ID_VIEW_FRAMES, OnViewFrames )
	ON_COMMAND( ID_VIEW_SHOWSELECTION, OnViewShowselection )

	ON_COMMAND( ID_FILE_PRINT, &CView::OnFilePrint )
	ON_COMMAND( ID_FILE_PRINT_DIRECT, &CView::OnFilePrint )
	ON_COMMAND( ID_FILE_PRINT_PREVIEW, &CView::OnFilePrintPreview )
END_MESSAGE_MAP()

int CBrowserView::OnCreate( CREATESTRUCT* pCreateStruct )
{
	if ( -1 == CView::OnCreate( pCreateStruct ) )
		return -1;

	CRect browserRect;
	GetClientRect( &browserRect );
	browserRect.DeflateRect( 10, 10 );		// no effect, really

	if ( SUCCEEDED( ::SHCoCreateInstance( NULL, &CLSID_ExplorerBrowser, NULL, IID_PPV_ARGS( &m_pExplorerBrowser ) ) ) )
	{
		if ( g_theApp.m_showFrames )
			m_pExplorerBrowser->SetOptions( EBO_SHOWFRAMES );

		FOLDERVIEWMODE folderViewMode = static_cast< FOLDERVIEWMODE >( AfxGetApp()->GetProfileInt( reg::section_view, reg::entry_viewMode, FVM_DETAILS ) );
		FOLDERSETTINGS settings = { folderViewMode, 0 };
		if ( SUCCEEDED( m_pExplorerBrowser->Initialize( m_hWnd, &browserRect, &settings ) ) )
		{
			CComObject< CExplorerBrowserEvents >* pExplorerEvents;
			if ( SUCCEEDED( CComObject< CExplorerBrowserEvents >::CreateInstance( &pExplorerEvents ) ) )
			{
				pExplorerEvents->AddRef();

				pExplorerEvents->SetView( this );
				m_pExplorerBrowser->Advise( pExplorerEvents, &m_dwAdviseCookie );

				pExplorerEvents->Release();
				return 0;
			}
		}
	}

	return -1;
}

void CBrowserView::OnDestroy( void )
{
	AfxGetApp()->WriteProfileInt( reg::section_view, reg::entry_viewMode, shell::GetFolderViewMode( m_pExplorerBrowser ) );
	CView::OnDestroy();

	if( m_dwAdviseCookie )
		m_pExplorerBrowser->Unadvise( m_dwAdviseCookie );

	m_pExplorerBrowser->Destroy();
}

void CBrowserView::OnInitialUpdate( void )
{
	CView::OnInitialUpdate();

	// navigate to the current directory
	TCHAR currDirPath[ _MAX_PATH ] = { _T('\0') };

	if ( GetCurrentDirectory( _MAX_PATH, currDirPath ) )
		if ( shell::NavigateTo( currDirPath, m_pExplorerBrowser ) )
			return;

	OnBrowseToProfileFolder();
}

void CBrowserView::OnSize( UINT nType, int cx, int cy )
{
	CView::OnSize(nType, cx, cy);

	m_pExplorerBrowser->SetRect(NULL, CRect(0, 0, cx, cy));
}

void CBrowserView::OnDraw( CDC* pDC )
{
	pDC;
}

BOOL CBrowserView::OnPreparePrinting( CPrintInfo* pInfo )
{
	return DoPreparePrinting( pInfo );
}

void CBrowserView::OnBeginPrinting( CDC* /*pDC*/, CPrintInfo* /*pInfo*/ )
{
}

void CBrowserView::OnEndPrinting( CDC* /*pDC*/, CPrintInfo* /*pInfo*/ )
{
}

void CBrowserView::OnBrowseToProfileFolder( void )
{
	HRESULT hr = S_OK;
	LPITEMIDLIST pidlBrowse = NULL;
	if ( SUCCEEDED( hr = SHGetFolderLocation( NULL, CSIDL_PROFILE, NULL, 0, &pidlBrowse ) ) )
	{
		if ( FAILED( hr = m_pExplorerBrowser->BrowseToIDList( pidlBrowse, 0 ) ) )
			TRACE( "BrowseToIDList Failed! hr = %d\n", hr );

		ILFree( pidlBrowse );
	}
	else
		TRACE("SHGetFolderLocation Failed! hr = %d\n", hr);

	if ( FAILED( hr ) )
		AfxMessageBox( _T("Navigation failed!") );
}

void CBrowserView::OnViewMode( UINT cmdId )
{
	FOLDERVIEWMODE folderViewMode = shell::CmdToViewMode( cmdId );
	FOLDERSETTINGS settings = { folderViewMode, 0 };

	HRESULT hr = m_pExplorerBrowser->SetFolderSettings( &settings );
	if ( FAILED( hr ) )
		AfxMessageBox( _T("SetFolderSettings() failed!") );
}

void CBrowserView::OnUpdateViewMode( CCmdUI* pCmdUI )
{
	UINT folderFlags;
	FOLDERVIEWMODE selViewMode = shell::GetFolderViewMode( m_pExplorerBrowser, &folderFlags );

	ui::SetRadio( pCmdUI, selViewMode == shell::CmdToViewMode( pCmdUI->m_nID ) );
}

void CBrowserView::OnViewBack( void )
{
	m_pExplorerBrowser->BrowseToIDList( NULL, SBSP_NAVIGATEBACK );
}

void CBrowserView::OnViewForward( void )
{
	m_pExplorerBrowser->BrowseToIDList( NULL, SBSP_NAVIGATEFORWARD );
}

void CBrowserView::OnViewFrames( void )
{
	m_showFrames = !m_showFrames;
	m_pExplorerBrowser->SetOptions( m_showFrames ? EBO_SHOWFRAMES : EBO_NONE );
}

void CBrowserView::OnViewShowselection( void )
{
	std::vector< std::tstring > selPaths;
	shell::QuerySelectedFiles( selPaths, m_pExplorerBrowser );

	CString message;
	message.Format( _T("Selected %d files:\n%s"), selPaths.size(), str::Join( selPaths, _T("\n") ).c_str() );

	AfxMessageBox( message );
}
