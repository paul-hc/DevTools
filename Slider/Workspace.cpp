
#include "stdafx.h"
#include "Workspace.h"
#include "WorkspaceDialog.h"
#include "SlideData.h"
#include "MainFrame.h"
#include "MainToolbar.h"
#include "ImageView.h"
#include "ChildFrame.h"
#include "Application.h"
#include "resource.h"
#include "utl/Serialization.h"
#include "utl/SerializeStdTypes.h"
#include "utl/UI/ImagingDirect2D.h"
#include "utl/UI/MfcUtilities.h"
#include "utl/UI/ShellUtilities.h"
#include "utl/UI/Utilities.h"
#include "utl/UI/Thumbnailer.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace reg
{
	const TCHAR section_Workspace[] = _T("Settings\\Workspace");
	const TCHAR entry_enlargeInterpolationMode[] = _T("EnlargeImageSmoothing");
	const TCHAR entry_defaultSlideDelay[] = _T("DefaultSlideDelay");
}


// CWorkspaceData struct

CWorkspaceData::CWorkspaceData( void )
	: m_autoSave( false )
	, m_wkspFlags( wf::DefaultFlags )
	, m_scalingMode( ui::AutoFitLargeOnly )
	, m_albumViewFlags( af::DefaultFlags )
	, m_mruCount( 10 )
	, m_defBkColor( color::VeryDarkGrey )
	, m_imageSelColor( color::Null )
	, m_imageSelTextColor( color::Null )
	, m_thumbListColCount( 1 )
	, m_thumbBoundsSize( app::GetThumbnailer()->GetBoundsSize().cx )
{
}

void CWorkspaceData::Save( CArchive& archive )
{
	ASSERT( archive.IsStoring() );

	// note: bool serializes as BOOL with streaming operator&()

	BOOL versionAsBool = static_cast< BOOL >( app::Slider_LatestModelSchema );

	archive << versionAsBool;				// save as BOOL for backwards compat: could be CWorkspace::m_autoSave (bool) or (slider::ModelSchema)

	std::string versionTag = str::AsNarrow( app::FormatSliderVersion( app::Slider_LatestModelSchema ) + _T(" Workspace") );
	archive << versionTag;					// user-readable version tag

	archive & m_autoSave;
	archive << m_wkspFlags;
	archive << m_albumViewFlags;
	archive << m_mruCount;
	archive << m_thumbListColCount;
	archive << m_defBkColor;
	archive << m_imageSelColor;
	archive << m_imageSelTextColor;

	archive << m_thumbBoundsSize;
	archive << (int&)m_scalingMode;
}

app::ModelSchema CWorkspaceData::Load( CArchive& archive )
{
	ASSERT( archive.IsLoading() );

	// hack for backwards compatibility
	BOOL versionAsBool;
	archive >> versionAsBool;						// save as BOOL for backwards compat: could be CWorkspace::m_autoSave

	app::ModelSchema savedModelSchema = static_cast< app::ModelSchema >( versionAsBool );

	if ( FALSE == versionAsBool || TRUE == versionAsBool )		// old workspace format?
		m_autoSave = versionAsBool != FALSE;		// used to be streamed first; skip loading, will load with the old format
	else
	{
		std::string versionTag;
		archive >> versionTag;						// discard the user-readable version tag

		archive & m_autoSave;
		archive >> m_wkspFlags;
		archive >> m_albumViewFlags;
		archive >> m_mruCount;
		archive >> m_thumbListColCount;
		archive >> m_defBkColor;
		archive >> m_imageSelColor;
		archive >> m_imageSelTextColor;

		if ( savedModelSchema >= app::Slider_v3_6 )
			archive >> m_thumbBoundsSize;

		if ( savedModelSchema >= app::Slider_v4_0 )
			archive >> (int&)m_scalingMode;
		else
			m_scalingMode = HasFlag( m_wkspFlags, wf::Old_InitStretchToFit ) ? ui::AutoFitLargeOnly : ui::ActualSize;
	}
	return savedModelSchema;
}


// CWorkspace implementation

CWorkspace::CWorkspace( void )
	: CCmdTarget()
	, m_pMainFrame( app::GetMainFrame() )
	, m_isLoaded( false )
	, m_delayFullScreen( false )
	, m_pLoadingImageState( NULL )
	, m_isFullScreen( false )
	, m_reserved( 0 )
	, m_defaultSlideDelay( 5000 )
{
	fs::CPathParts parts( app::GetModulePath().Get() );
	parts.m_ext = _T(".slw");
	m_filePath = parts.MakePath();

	SetImageSelColor( color::Null );	// also create the associated brush
}

CWorkspace::~CWorkspace()
{
	m_imageStates.clear();
}

CWorkspace& CWorkspace::Instance( void )
{
	static CWorkspace appWorkspace;
	return appWorkspace;
}

void CWorkspace::Serialize( CArchive& archive )
{
	if ( archive.IsStoring() )
	{	// save workspace in the latest format
		m_data.Save( archive );

		archive & m_isFullScreen;
		archive << m_mainPlacement;
		archive << m_reserved;
	}
	else
	{
		app::ModelSchema savedModelSchema = m_data.Load( archive );

		if ( savedModelSchema >= app::Slider_v3_5 )
		{
			archive & m_delayFullScreen;			// temporary replacer for m_isFullScreen
			archive >> m_mainPlacement;
			archive >> m_reserved;
		}
		else
		{	// backwards compat: load with the old format, including the real saved version; don't change the loading order

			// note: m_data.m_autoSave was already loaded, skip it
			archive >> m_data.m_wkspFlags;
			archive >> m_data.m_albumViewFlags;
			archive & m_delayFullScreen;			// temporary replacer for m_isFullScreen
			archive >> m_data.m_mruCount;
			archive >> m_data.m_defBkColor;
			archive >> m_data.m_imageSelColor;
			archive >> m_data.m_imageSelTextColor;
			archive >> m_data.m_thumbListColCount;
			archive >> m_mainPlacement;
			archive >> (int&)savedModelSchema;			// the real saved old version
			archive >> m_reserved;
		}

		SetImageSelColor( m_data.m_imageSelColor );

		// workspace loaded: will use SW_HIDE on 1st show (CommitWnd), then pass the final mode from placement on 2nd step for MFC to show
		m_isLoaded = true;
		AfxGetApp()->m_nCmdShow = m_mainPlacement.ChangeMaximizedShowCmd( SW_HIDE );
		shell::s_useVistaStyle = HasFlag( m_data.m_wkspFlags, wf::UseVistaStyleFileDialog );

		if ( app::GetThumbnailer()->SetBoundsSize( m_data.GetThumbBoundsSize() ) )
			app::GetApp()->UpdateAllViews( Hint_ThumbBoundsResized );			// notify thumb bounds change

		if ( savedModelSchema < app::Slider_v3_2 )
			return;									// skip loading old m_imageStates that were using DECLARE_SERIAL( CImageState )
	}

	serial::StreamItems( archive, m_imageStates );
}

bool CWorkspace::LoadSettings( void )
{
	LoadRegSettings();

	ASSERT( !m_isLoaded );		// load once
	ui::CAdapterDocument doc( this, m_filePath );
	return doc.Load();			// load the workspace file
}

bool CWorkspace::SaveSettings( void )
{
	SaveRegSettings();

	ui::CAdapterDocument doc( this, m_filePath );
	return doc.Save();
}

void CWorkspace::LoadRegSettings( void )
{
	m_defaultSlideDelay = AfxGetApp()->GetProfileInt( reg::section_Workspace, reg::entry_defaultSlideDelay, m_defaultSlideDelay );

	d2d::CDrawBitmapTraits::s_enlargeInterpolationMode =
		static_cast< D2D1_BITMAP_INTERPOLATION_MODE >( AfxGetApp()->GetProfileInt( reg::section_Workspace, reg::entry_enlargeInterpolationMode, d2d::CDrawBitmapTraits::s_enlargeInterpolationMode ) );
}

void CWorkspace::SaveRegSettings( void )
{
	AfxGetApp()->WriteProfileInt( reg::section_Workspace, reg::entry_defaultSlideDelay, m_defaultSlideDelay );
	AfxGetApp()->WriteProfileInt( reg::section_Workspace, reg::entry_enlargeInterpolationMode, d2d::CDrawBitmapTraits::s_enlargeInterpolationMode );
}

// check and adjust application's forced flags
void CWorkspace::AdjustForcedBehaviour( void )
{
	const CApplication* pApp = app::GetApp();

	if ( pApp->HasForceMask( app::FullScreen ) )
		m_delayFullScreen = pApp->HasForceFlag( app::FullScreen );

	if ( pApp->HasForceMask( app::DocMaximize ) )
		SetFlag( m_data.m_wkspFlags, wf::MdiMaximized, pApp->HasForceFlag( app::DocMaximize ) );

	if ( pApp->HasForceMask( app::ShowToolbar ) )
		SetFlag( m_data.m_wkspFlags, wf::ShowToolBar, pApp->HasForceFlag( app::ShowToolbar ) );

	if ( pApp->HasForceMask( app::ShowStatusBar ) )
		SetFlag( m_data.m_wkspFlags, wf::ShowStatusBar, pApp->HasForceFlag( app::ShowStatusBar ) );
}

void CWorkspace::FetchSettings( void )
{
	// re-read main window placement only if not in full-screen mode;
	// if it is in full-screen, we assume that m_mainPlacement is already fetched (on switch time) with appropriate info.
	if ( !m_isFullScreen )
		m_mainPlacement.ReadWnd( m_pMainFrame );

	BOOL isMaximized;
	CChildFrame* pActiveChildFrame = checked_static_cast< CChildFrame* >( m_pMainFrame->MDIGetActive( &isMaximized ) );
	if ( pActiveChildFrame != NULL )				// any MDI child available
		SetFlag( m_data.m_wkspFlags, wf::MdiMaximized, isMaximized != FALSE );		// save MDI maximize state

	// save toolbar and status-bar visibility state
	SetFlag( m_data.m_wkspFlags, wf::ShowToolBar, ui::IsVisible( m_pMainFrame->GetToolbar()->GetSafeHwnd() ) );
	SetFlag( m_data.m_wkspFlags, wf::ShowStatusBar, ui::IsVisible( m_pMainFrame->GetStatusBar()->GetSafeHwnd() ) );

	m_imageStates.clear();

	if ( HasFlag( m_data.m_wkspFlags, wf::PersistOpenDocs ) )
		if ( pActiveChildFrame != NULL )
			for ( CWnd* pChild = pActiveChildFrame->GetNextWindow( GW_HWNDLAST ); pChild != NULL; pChild = pChild->GetNextWindow( GW_HWNDPREV ) )
			{
				CChildFrame* pChildFrame = checked_static_cast< CChildFrame* >( pChild );
				fs::CPath docFilePath( pChildFrame->GetActiveDocument()->GetPathName().GetString() );

				if ( !docFilePath.IsEmpty() )
					if ( IImageView* pImageView = pChildFrame->GetImageView() )			// ensure not a print preview, etc
					{
						m_imageStates.push_back( CImageState() );
						dynamic_cast< const CImageView* >( pChildFrame->GetImageView()->GetView() )->MakeImageState( &m_imageStates.back() );
					}
			}
}

bool CWorkspace::LoadDocuments( void )
{
	if ( m_delayFullScreen && m_delayFullScreen != m_isFullScreen )
		ToggleFullScreen();		// if was de-persisted with full-screen mode, now is the right time to actually commit the switch

	// (!) next time, show the window as default in order to properly handle CFrameWnd::OnDDEExecute ShowWindow calls
	AfxGetApp()->m_nCmdShow = -1;

	if ( !m_pMainFrame->GetToolbar()->IsVisible() == HasFlag( m_data.m_wkspFlags, wf::ShowToolBar ) )
		m_pMainFrame->ShowControlBar( m_pMainFrame->GetToolbar(), HasFlag( m_data.m_wkspFlags, wf::ShowToolBar ), FALSE );
	if ( !m_pMainFrame->GetStatusBar()->IsVisible() == HasFlag( m_data.m_wkspFlags, wf::ShowStatusBar ) )
		m_pMainFrame->ShowControlBar( m_pMainFrame->GetStatusBar(), HasFlag( m_data.m_wkspFlags, wf::ShowStatusBar ), FALSE );

	ASSERT_NULL( m_pLoadingImageState );
	for ( std::vector< CImageState >::const_iterator itImageState = m_imageStates.begin(); itImageState != m_imageStates.end(); ++itImageState )
	{
		// newly created view will initialize it's visual state (frame placement, zoom, etc) on initial update based on m_pLoadingImageState
		//
		m_pLoadingImageState = const_cast< CImageState* >( &*itImageState );

		if ( NULL == AfxGetApp()->OpenDocumentFile( m_pLoadingImageState->GetDocFilePath().c_str() ) )
			TRACE( " * Failed loading document %s on workspace load!\n", m_pLoadingImageState->GetDocFilePath().c_str() );
	}
	m_pLoadingImageState = NULL;
	m_imageStates.clear();				// no longer needed

	return !m_imageStates.empty();
}

void CWorkspace::ToggleFullScreen( void )
{
	m_isFullScreen = !m_isFullScreen;

	if ( !m_isFullScreen )
		m_mainPlacement.CommitWnd( m_pMainFrame );		// restore to normal state
	else
	{
		// store current placement for normal state (in order to be further resored).
		m_mainPlacement.ReadWnd( m_pMainFrame );

		// window is about to compute to full screen mode -> so compute the rect for full screen state
		if ( m_pMainFrame->IsZoomed() )
			m_pMainFrame->ShowWindow( SW_RESTORE );

		CRect mainWndRect;
		m_pMainFrame->GetWindowRect( &mainWndRect );

		CRect mdiClientScreenRect;
		::GetClientRect( m_pMainFrame->m_hWndMDIClient, &mdiClientScreenRect );
		ui::ClientToScreen( m_pMainFrame->m_hWndMDIClient, mdiClientScreenRect );

		// compute the MDI-Client to main-frame difference rectangle
		CRect deltaRect;
		deltaRect.left = mdiClientScreenRect.left - mainWndRect.left;
		deltaRect.top = mdiClientScreenRect.top - mainWndRect.top;
		deltaRect.right = mainWndRect.right - mdiClientScreenRect.right;
		deltaRect.bottom = mainWndRect.bottom - mdiClientScreenRect.bottom;

		mainWndRect = ui::FindMonitorRect( m_pMainFrame->m_hWnd, ui::Monitor );
		//::GetWindowRect( ::GetDesktopWindow(), &mainWndRect );
		mainWndRect.InflateRect( &deltaRect );

		CSize borderSize( GetSystemMetrics( SM_CXEDGE ), GetSystemMetrics( SM_CYEDGE ) );

		mainWndRect.InflateRect( borderSize );

		// note that SWP_NOSENDCHANGING flag is mandatory since WM_WINDOWPOSCHANGING limits main frame's position
		m_pMainFrame->SetWindowPos( &CWnd::wndTop, mainWndRect.left, mainWndRect.top, mainWndRect.Width(), mainWndRect.Height(),
			/*SWP_NOOWNERZORDER | SWP_NOZORDER |*/ SWP_SHOWWINDOW | SWP_NOSENDCHANGING );
	}

	// notify all views that full screen mode has changed
	app::GetApp()->UpdateAllViews( Hint_ToggleFullScreen );
}

void CWorkspace::SetImageSelColor( COLORREF imageSelColor )
{
	m_data.m_imageSelColor = imageSelColor;
	m_imageSelColorBrush.DeleteObject();
	m_imageSelColorBrush.CreateSolidBrush( GetImageSelColor() );

	// set the selected text color
	if ( color::Null == m_data.m_imageSelColor )
		m_data.m_imageSelTextColor = color::Null;
}


// commands handlers

BEGIN_MESSAGE_MAP( CWorkspace, CCmdTarget )
	ON_COMMAND( CM_SAVE_WORKSPACE, CmSaveWorkspace )
	ON_COMMAND( CM_EDIT_WORKSPACE, CmEditWorkspace )
	ON_COMMAND( CM_LOAD_WORKSPACE_DOCS, CmLoadWorkspaceDocs )
	ON_COMMAND( IDW_SMOOTHING_MODE_CHECK, OnToggle_SmoothingMode )
	ON_UPDATE_COMMAND_UI( IDW_SMOOTHING_MODE_CHECK, OnUpdate_SmoothingMode )
END_MESSAGE_MAP()

void CWorkspace::CmLoadWorkspaceDocs( void )
{
	LoadDocuments();
}

void CWorkspace::CmSaveWorkspace( void )
{
	FetchSettings();
	SaveSettings();
}

void CWorkspace::CmEditWorkspace( void )
{
	CWorkspaceDialog dlg( m_pMainFrame );
	INT_PTR result = dlg.DoModal();

	if ( result != IDCANCEL )
	{
		std::pair< UpdateViewHint, bool > changed( Hint_Null, false );		// <hint, save>
		if ( dlg.m_data != m_data )
		{
			m_data = dlg.m_data;
			SetImageSelColor( m_data.m_imageSelColor );
			shell::s_useVistaStyle = HasFlag( m_data.m_wkspFlags, wf::UseVistaStyleFileDialog );

			changed = std::make_pair( Hint_ViewUpdate, true );
		}
		if ( d2d::CDrawBitmapTraits::SetSmoothingMode( dlg.m_enlargeSmoothing ) )
		{
			SaveRegSettings();
			changed.first = Hint_ViewUpdate;
		}
		if ( dlg.m_defaultSlideDelay != m_defaultSlideDelay )
		{
			m_defaultSlideDelay = dlg.m_defaultSlideDelay;
			SaveRegSettings();
			changed.first = Hint_ViewUpdate;
		}

		CThumbnailer* pThumbnailer = app::GetThumbnailer();

		if ( utl::ModifyValue( pThumbnailer->m_flags, dlg.m_thumbnailerFlags ) )
			changed = std::make_pair( Hint_ViewUpdate, true );

		if ( pThumbnailer->SetBoundsSize( m_data.GetThumbBoundsSize() ) )
			changed = std::make_pair( Hint_ThumbBoundsResized, true );

		if ( changed.first != Hint_Null )
			app::GetApp()->UpdateAllViews( changed.first );

		if ( changed.second && result != CM_SAVE_WORKSPACE )
			if ( IDYES == AfxMessageBox( _T("Save changed workspace preferences?"), MB_YESNO | MB_ICONQUESTION ) )
				result = CM_SAVE_WORKSPACE;
	}

	if ( CM_SAVE_WORKSPACE == result )
		CmSaveWorkspace();
}

void CWorkspace::OnToggle_SmoothingMode( void )
{
	d2d::CDrawBitmapTraits::SetSmoothingMode( !d2d::CDrawBitmapTraits::IsSmoothingMode() );

	app::GetApp()->UpdateAllViews( Hint_RedrawAll );
}

void CWorkspace::OnUpdate_SmoothingMode( CCmdUI* pCmdUI )
{
	pCmdUI->SetCheck( d2d::CDrawBitmapTraits::IsSmoothingMode() );
}
