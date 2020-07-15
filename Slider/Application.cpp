
#include "stdafx.h"
#include "Application.h"
#include "DocTemplates.h"
#include "MainFrame.h"
#include "Workspace.h"
#include "AlbumDoc.h"
#include "IImageView.h"
#include "ICatalogStorage.h"
#include "MoveFileDialog.h"
#include "OleImagesDataSource.h"
#include "test/CatalogStorageTests.h"
#include "test/ImagingD2DTests.h"
#include "test/ThumbnailTests.h"
#include "resource.h"
#include "utl/RuntimeException.h"
#include "utl/StringUtilities.h"
#include "utl/UI/AboutBox.h"
#include "utl/UI/DragListCtrl.h"
#include "utl/UI/GdiPlus_fwd.h"
#include "utl/UI/MfcUtilities.h"
#include "utl/UI/ShellDialogs.h"
#include "utl/UI/ShellUtilities.h"
#include "utl/UI/UtilitiesEx.h"
#include "utl/UI/Thumbnailer.h"
#include "utl/UI/WicImageCache.h"
#include "utl/UI/test/WicImageTests.h"
#include <io.h>

///
#include "AlbumChildFrame.h"
#include "AlbumImageView.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#include "utl/UI/BaseApp.hxx"


static const CImageStore::CCmdAlias cmdAliases[] =
{
	{ ID_IMAGE_OPEN, ID_FILE_OPEN },
	{ ID_IMAGE_SAVE_AS, ID_FILE_SAVE },
	{ ID_IMAGE_DELETE, ID_REMOVE_ITEM },
	{ ID_EDIT_ALBUM, ID_EDIT_ITEM },
	{ ID_EDIT_ARCHIVE_PASSWORD, IDD_PASSWORD_DIALOG },
	{ CM_REFRESH_CONTENT, ID_REFRESH },
	{ ID_FILE_OPEN_ALBUM_FOLDER, ID_BROWSE_FOLDER },
	{ CM_CUSTOM_ORDER_UNDO, ID_EDIT_UNDO },
	{ CM_CUSTOM_ORDER_REDO, ID_EDIT_REDO },

	// * special case: toolbar buttons that use controls must have a placeholder image associated; otherwise the image list gets shifted completely
	{ IDW_IMAGE_SCALING_COMBO, ID_EDIT_DETAILS },
	{ IDW_ZOOM_COMBO, ID_EDIT_DETAILS },
	{ IDW_NAVIG_SLIDER_CTRL, ID_EDIT_DETAILS }
};


namespace app
{
	void LogLine( const TCHAR* pFormat, ... )
	{
		va_list argList;

		va_start( argList, pFormat );
		theApp.GetLogger().LogV( pFormat, argList );
		va_end( argList );
	}

	void LogEvent( const TCHAR* pFormat, ... )
	{
		va_list argList;

		va_start( argList, pFormat );
		theApp.GetEventLogger().LogV( pFormat, argList );
		va_end( argList );
	}

	void HandleException( CException* pExc, UINT mbType /*= MB_ICONWARNING*/, bool doDelete /*= true*/ )
	{
		ASSERT_PTR( pExc );

		if ( is_a< mfc::CUserAbortedException >( pExc ) )
			return;					// already reported, skip logging

		std::tstring message = _T("* C++ MFC exception: ");
		message += mfc::CRuntimeException::MessageOf( *pExc );
		TRACE( _T("%s\n"), message.c_str() );
		LogLine( message.c_str() );

		if ( mbType != MB_OK )
			ui::BeepSignal( mbType );

		if ( doDelete )
			pExc->Delete();
	}

	int HandleReportException( CException* pExc, UINT mbType /*= MB_ICONERROR*/, UINT msgId /*= 0*/, bool doDelete /*= true*/ )
	{
		ASSERT_PTR( pExc );
		HandleException( pExc, MB_OK, false );
		int result = pExc->ReportError( mbType, msgId );

		if ( doDelete )
			pExc->Delete();
		return result;
	}


	ui::IUserReport& GetUserReport( void )
	{
		return app::CInteractiveMode::Instance();
	}

	ui::CIssueStore& GetIssueStore( void )
	{
		static ui::CIssueStore s_issueStore;
		return s_issueStore;
	}

	bool IsCatalogFile( const TCHAR* pFilePath )
	{
		return CCatalogStorageFactory::HasCatalogExt( pFilePath );
	}

	bool IsSlideFile( const TCHAR* pFilePath )
	{
		return CAlbumDocTemplate::IsSlideAlbumFile( pFilePath );
	}

	fs::CPath GetActiveDirPath( void )
	{
		fs::CPath activePath;

		if ( IImageView* pActiveView = app::GetMainFrame()->GetActiveImageView() )
		{
			activePath = pActiveView->GetImagePathKey().first.GetPhysicalPath();

			if ( fs::IsValidFile( activePath.GetPtr() ) )
				activePath = activePath.GetParentPath();

			if ( !fs::IsValidDirectory( activePath.GetPtr() ) )
				activePath.Clear();
		}
		return activePath;
	}



	// CScopedUserReport implementation

	ui::IUserReport* CScopedUserReport::s_pUserReport = &app::CInteractiveMode::Instance();


	// CInteractiveMode implementation

	ui::IUserReport& CInteractiveMode::Instance( void )
	{
		static CInteractiveMode s_interactiveMode;
		return s_interactiveMode;
	}

	int CInteractiveMode::ReportError( CException* pExc, UINT mbType /*= MB_OK*/ )
	{
		return HandleReportException( pExc, mbType );
	}


	// CScopedProgress implementation

///	CScopedProgress::CScopedProgress( int autoClearDelay /*= 250*/ )
///		: m_pSharedProgressBar( app::GetMainFrame()->GetProgressCtrl() )
///		, m_autoClearDelay( autoClearDelay )
///		, m_pbStepIndex( 0 )
///		, m_pbStepDivider( 1 )
///	{
///	}
///
///	CScopedProgress::CScopedProgress( int valueMin, int count, int stepCount, const TCHAR* pCaption /*= NULL*/, int autoClearDelay /*= 250*/ )
///		: m_pSharedProgressBar( app::GetMainFrame()->GetProgressCtrl() )
///		, m_autoClearDelay( autoClearDelay )
///		, m_pbStepIndex( 0 )
///		, m_pbStepDivider( 1 )
///		, m_pMessagePump( new CScopedPumpMessage( 5, CWnd::GetActiveWindow() ) )
///	{
///		Begin( valueMin, count, stepCount, pCaption );
///	}
///
///	CScopedProgress::~CScopedProgress()
///	{
///		if ( m_autoClearDelay != ACD_NoClear )
///			End();
///	}
///
///	bool CScopedProgress::IsActive( void ) const
///	{
///		return app::GetMainFrame()->InProgress();
///	}
///
///	void CScopedProgress::Begin( int valueMin, int count, int stepCount, const TCHAR* pCaption /*= NULL*/ )
///	{
///		app::GetMainFrame()->BeginProgress( valueMin, count, stepCount, pCaption );
///	}
///
///	void CScopedProgress::End( int clearDelay /*= ACD_NoClear*/ )
///	{
///		app::GetMainFrame()->EndProgress( clearDelay == ACD_NoClear ? m_autoClearDelay : clearDelay );
///	}
///
///	void CScopedProgress::SetPos( int value )
///	{
///		app::GetMainFrame()->SetPosProgress( value );
///	}
///
///	void CScopedProgress::StepIt( void )
///	{
///		// if m_pbStepDivider > 1 divide StepIt calls by m_pbStepDivider
///		if ( m_pbStepDivider <= 1 || !( ++m_pbStepIndex % m_pbStepDivider ) )
///			app::GetMainFrame()->StepItProgress();
///
///		if ( m_pMessagePump.get() != NULL )
///			m_pMessagePump->CheckPump();
///	}
///
///	void CScopedProgress::GotoBegin( void )
///	{
///		int valueMin, valueMax;
///		ASSERT( IsActive() );
///		m_pSharedProgressBar->GetRange( valueMin, valueMax );
///		m_pSharedProgressBar->SetPos( valueMin );
///	}
///
///	void CScopedProgress::GotoEnd( void )
///	{
///		int valueMin, valueMax;
///		ASSERT( IsActive() );
///		m_pSharedProgressBar->GetRange( valueMin, valueMax );
///		m_pSharedProgressBar->SetPos( valueMax - 1 );
///	}

} //namespace app


namespace app
{
	const std::tstring& GetAllSourcesWildSpecs( void )
	{
		static const std::tstring s_wildFilters = CSliderFilters::Instance().MakeSpecs( shell::FileOpen );
		return s_wildFilters;
	}

	bool BrowseAlbumFile( fs::CPath& rFullPath, CWnd* pParentWnd, shell::BrowseMode browseMode /*= shell::FileOpen*/, DWORD flags /*= 0*/ )
	{
		static const std::tstring stgFilters = CAlbumFilterStore::Instance().MakeCatalogStgFilters();
		return shell::BrowseForFile( rFullPath, pParentWnd, browseMode, stgFilters.c_str(), flags );
	}

	bool BrowseCatalogFile( fs::CPath& rFullPath, CWnd* pParentWnd, shell::BrowseMode browseMode /*= shell::FileOpen*/, DWORD flags /*= 0*/ )
	{
		static const std::tstring stgFilters = CAlbumFilterStore::Instance().MakeCatalogStgFilters();
		return shell::BrowseForFile( rFullPath, pParentWnd, browseMode, stgFilters.c_str(), flags );
	}

} //namespace shell


// Slider application

namespace reg
{
	const TCHAR section_Settings[] = _T("Settings");
	const TCHAR entry_CustomColors[] = _T("CustomColors");
	const TCHAR entry_WorkDir[] = _T("LastWorkingDir");
}

CApplication theApp;

CApplication::CApplication( void )
	: CBaseApp< CWinApp >()
	, m_pMainFrame( NULL )
	, m_runFlags( 0 )
	, m_forceMask( 0 )
	, m_forceFlags( 0 )
	, m_pEventLogger( new CLogger( _T("%s-events") ) )
{
	COLORREF* pCustomColors = CColorDialog::GetSavedCustomColors();
	UINT component = 0x00, increment = 0xFF / 15;

	for ( int i = 0; i != 16; ++i, component += increment, component = std::min( component, 0xFFu ) )
		pCustomColors[ i ] = RGB( component, component, component );

	// TODO: add construction code here; place all significant initialization in InitInstance
}

CApplication::~CApplication()
{
}

BOOL CApplication::InitInstance( void )
{
//	m_pGdiPlusInit.reset( new CScopedGdiPlusInit );

	if ( !CBaseApp< CWinApp >::InitInstance() )
		return FALSE;

LoadStdProfileSettings( _AFX_MRU_MAX_COUNT );		// load standard INI file options (including MRU)

	serial::CScopedLoadingArchive::SetLatestModelSchema( app::Slider_LatestModelSchema );			// set up the latest model schema version (assumed by default for in-memory serialization)

	GetLogger().m_enabled = GetProfileInt( reg::section_Settings, _T("FileLogger_Enabled"), false ) != FALSE;			// disable logging by default
	GetLogger().m_prependTimestamp = GetProfileInt( reg::section_Settings, _T("FileLogger_PrependTimestamp"), GetLogger().m_prependTimestamp ) != FALSE;
	GetEventLogger().m_enabled = GetProfileInt( reg::section_Settings, _T("EventLogger_Enabled"), false ) != FALSE;		// disable logging by default
	GetEventLogger().m_prependTimestamp = GetProfileInt( reg::section_Settings, _T("EventLogger_PrependTimestamp"), GetEventLogger().m_prependTimestamp ) != FALSE;

	std::vector< COLORREF > customColors;
	if ( app::GetProfileVector( customColors, reg::section_Settings, reg::entry_CustomColors ) )
	{
		ASSERT( 16 == customColors.size() );
		memcpy( CColorDialog::GetSavedCustomColors(), &customColors.front(), customColors.size() * sizeof( COLORREF ) );
	}

	m_pThumbnailer.reset( new CThumbnailer );
	GetSharedResources().AddAutoPtr( &m_pThumbnailer );
	GetSharedResources().AddAutoClear( &CWicImageCache::Instance() );
	m_pThumbnailer->SetExternalProducer( CCatalogStorageFactory::Instance() );		// add as producer of storage-based thumbnails

	CAboutBox::m_appIconId = IDR_MAINFRAME;
	m_sharedAccel.Load( IDR_COMMAND_BAR_ACCEL );
	CToolStrip::RegisterStripButtons( IDR_MAINFRAME );
	CToolStrip::RegisterStripButtons( IDR_APP_TOOL_STRIP );
	CImageStore::SharedStore()->RegisterAliases( cmdAliases, COUNT_OF( cmdAliases ) );

/**	app::CDocManager* pAppDocManager = new app::CDocManager;
	ASSERT_NULL( m_pDocManager );
	m_pDocManager = pAppDocManager;		// register document templates
**/
// Register the application's document templates.
CMultiDocTemplate* pDocTemplate;
pDocTemplate = new CMultiDocTemplate(IDR_ALBUMTYPE,
	RUNTIME_CLASS( CAlbumDoc ),
	RUNTIME_CLASS( CAlbumChildFrame ),		// custom MDI child frame
	RUNTIME_CLASS( CAlbumImageView )
);
AddDocTemplate(pDocTemplate);

///	CCmdLineInfo cmdInfo( this );
///	cmdInfo.ParseAppSwitches();				// just our switches (ignore MFC arguments)

///	if ( ui::IsKeyPressed( VK_SHIFT ) )
///		cmdInfo.SetForceFlag( app::FullScreen | app::DocMaximize, true );

///	if ( HasFlag( m_runFlags, ShowHelp ) )
///	{
///		AfxMessageBox( CCmdLineInfo::GetHelpMsg().c_str() );
///		return FALSE;
///	}

	// create main MDI Frame window
	m_pMainFrame = new CMainFrame();
	CWorkspace::Instance().StoreMainWnd( m_pMainFrame );

	if ( !m_pMainFrame->LoadFrame( IDR_MAINFRAME ) )
		return FALSE;

	m_pMainWnd = m_pMainFrame;

	// check and adjust application's forced flags
///	CWorkspace::Instance().AdjustForcedBehaviour();

	// adjust the MRU list max count
///	delete m_pRecentFileList;
///	m_pRecentFileList = NULL;
///	LoadStdProfileSettings( CWorkspace::GetData().m_mruCount );

///	ParseCommandLine( cmdInfo );			// parse command line for standard shell commands, DDE, file open

	EnableShellOpen();						// enable DDE Execute open
	RegisterShellFileTypes( TRUE );			// register image and album extensions and document types

CCommandLineInfo cmdInfo; // MFC-STD
	ParseCommandLine( cmdInfo );			// parse command line for standard shell commands, DDE, file open

///	if ( HasForceMask( app::RegAdditionalExt ) )
///		pAppDocManager->RegisterImageAdditionalShellExt( HasForceFlag( app::RegAdditionalExt ) );

///	if ( !cmdInfo.m_strFileName.IsEmpty() )
		// Dispatch commands specified on the command line.  Will return FALSE if app was launched with /RegServer, /Register, /Unregserver or /Unregister.
		if ( !ProcessShellCommand( cmdInfo ) )
			return FALSE;

///	if ( -1 == m_nCmdShow || SW_HIDE == m_nCmdShow )				// PHC 2019-01: in case we no longer use WM_DDE_EXECUTE command
///		m_nCmdShow = SW_SHOWNORMAL;		// make the main frame visible

	// the main window has been initialized, so show and update it
	m_pMainFrame->ShowWindow( m_nCmdShow );
	m_pMainFrame->UpdateWindow();

	if ( HandleAppTests() )
		return FALSE;

	return TRUE;
}

int CApplication::ExitInstance( void )
{
	TCHAR currWorkDir[ _MAX_DIR ];

	// save the last current working directory
	::GetCurrentDirectory( _MAX_DIR, currWorkDir );
	WriteProfileString( reg::section_Settings, reg::entry_WorkDir, currWorkDir );

	WriteProfileInt( reg::section_Settings, _T("FileLogger_Enabled"), GetLogger().m_enabled );
	WriteProfileInt( reg::section_Settings, _T("FileLogger_PrependTimestamp"), GetLogger().m_prependTimestamp );
	WriteProfileInt( reg::section_Settings, _T("EventLogger_Enabled"), GetEventLogger().m_enabled );
	WriteProfileInt( reg::section_Settings, _T("EventLogger_PrependTimestamp"), GetEventLogger().m_prependTimestamp );

	std::vector< COLORREF > customColors( CColorDialog::GetSavedCustomColors(), CColorDialog::GetSavedCustomColors() + 16 );
	app::WriteProfileVector( customColors, reg::section_Settings, reg::entry_CustomColors );

	m_pEventLogger.reset();

	return CBaseApp< CWinApp >::ExitInstance();
}

bool CApplication::_OpenQueuedAlbum( void )
{
	CAlbumDoc* pAlbumDoc = NULL;
	if ( m_pDocManager != NULL )
		pAlbumDoc = (CAlbumDoc*)app::CAlbumDocTemplate::Instance()->OpenDocumentFile( NULL );		// create a new album document

	ASSERT_PTR( pAlbumDoc );

	// show the application window
	int cmdShow = m_nCmdShow;
	if ( -1 == cmdShow || SW_SHOWNORMAL == cmdShow )
		cmdShow = m_pMainWnd->IsIconic() ? SW_RESTORE : SW_SHOW;
	m_pMainWnd->ShowWindow( cmdShow );
	if ( cmdShow != SW_MINIMIZE )
		m_pMainWnd->SetForegroundWindow();
	m_nCmdShow = -1;		// next time, show the window as default

	// add the queued files to the album
	pAlbumDoc->AddExplicitFiles( m_queuedAlbumFilePaths );
	pAlbumDoc->SetModifiedFlag( FALSE );		// initial non-modified (shell command)

	m_queuedAlbumFilePaths.clear();
	return true;
}

bool CApplication::HandleAppTests( void )
{
#ifdef _DEBUG
	// register Slider application's tests
	CCatalogStorageTests::Instance();

	if ( !HasFlag( m_runFlags, SkipUiTests ) )	// UI tests are not skipped?
	{
		CWicImageTests::Instance();
		CThumbnailTests::Instance();
		CImagingD2DTests::Instance();
	}

	if ( HasFlag( m_runFlags, RunTests ) )
	{
		RunUnitTests();
		return true;		// exit the application right away
	}
#endif
	return false;
}
/**
void CApplication::SetStatusBarMessage( const TCHAR* pMessage )
{
	m_pMainFrame->SetMessageText( pMessage );
} **/

void CApplication::UpdateAllViews( UpdateViewHint hint /*= Hint_ViewUpdate*/, CDocument* pSenderDoc /*= NULL*/, CView* pSenderView /*= NULL*/ )
{
	for ( POSITION templatePos = GetFirstDocTemplatePosition(); templatePos != NULL; )
		if ( CDocTemplate* pTempl = GetNextDocTemplate( templatePos ) )
			for ( POSITION docPos = pTempl->GetFirstDocPosition(); docPos != NULL; )
			{
				CDocument* pDoc = pTempl->GetNextDoc( docPos );
				if ( pDoc != NULL && pDoc != pSenderDoc )
					pDoc->UpdateAllViews( pSenderView, hint );	// Pass the 'hint' parameter to the view (pSenderView will be excepted from update)
			}
}

// Process special shell commands like:
//	[queue("D:\WINNT\Background\Tiles\JPGs\blue.jpg")]
BOOL CApplication::OnDDECommand( LPTSTR pCommand )
{
///	ASSERT(0);

/**	std::tstring command = pCommand;

	if ( str::StripPrefix( command, _T("[queue(") ) )
		if ( str::StripSuffix( command, _T(")]") ) )
		{	// process each DDE "queue" request for explicit albums
			str::StripPrefix( command, _T("\"") );
			str::StripSuffix( command, _T("\"") );

			m_queuedAlbumFilePaths.push_back( command );
			app::GetMainFrame()->StartQueuedAlbumTimer();
			return TRUE;
		} **/

	return CBaseApp< CWinApp >::OnDDECommand( pCommand );
}

BOOL CApplication::PreTranslateMessage( MSG* pMsg )
{
	if ( HWND hMainWnd = AfxGetMainWnd()->GetSafeHwnd() )
		if ( pMsg->hwnd == hMainWnd || ::IsChild( hMainWnd, pMsg->hwnd ) )
			if ( m_sharedAccel.Translate( pMsg, hMainWnd ) )
				return TRUE;

	return CBaseApp< CWinApp >::PreTranslateMessage( pMsg );
}


// message handlers

BEGIN_MESSAGE_MAP( CApplication, CBaseApp< CWinApp > )
	ON_COMMAND( ID_FILE_NEW, CBaseApp< CWinApp >::OnFileNew )
	ON_COMMAND( ID_FILE_OPEN, CBaseApp< CWinApp >::OnFileOpen )
	ON_COMMAND( ID_FILE_PRINT_SETUP, CBaseApp< CWinApp >::OnFilePrintSetup )
	ON_COMMAND( ID_FILE_OPEN_ALBUM_FOLDER, OnFileOpenAlbumFolder )
	ON_COMMAND( CM_CLEAR_TEMP_EMBEDDED_CLONES, OnClearTempEmbeddedClones )
END_MESSAGE_MAP()

void CApplication::OnFileOpenAlbumFolder( void )
{
	static fs::CPath s_folderPath;

	if ( s_folderPath.IsEmpty() || !fs::IsValidDirectory( s_folderPath.GetPtr() ) )
		s_folderPath = app::GetActiveDirPath();

	if ( shell::BrowseForFolder( s_folderPath, AfxGetMainWnd(), NULL, shell::BF_FileSystem, _T("Browse Folder Images"), false ) )
		OpenDocumentFile( s_folderPath.GetPtr() );
}

void CApplication::OnClearTempEmbeddedClones( void )
{
	CTempCloneFileSet::ClearAllTempFiles();
}


// CApplication::CCmdLineInfo implementation

void CApplication::CCmdLineInfo::ParseAppSwitches( void )
{
	ASSERT( __argc > 0 );

	for ( int i = 1; i != __argc; ++i )
	{
		const TCHAR* pParam = __targv[ i ];
		if ( arg::IsSwitch( pParam ) )
			ParseSwitch( arg::GetSwitch( pParam ) );
	}
}
/**
void CApplication::CCmdLineInfo::ParseParam( const TCHAR* pParam, BOOL isFlag, BOOL isLast )
{
	if ( isFlag )
		if ( ParseSwitch( pParam ) )
		{
			ParseLast( isLast );
			return;
		}

	CCommandLineInfo::ParseParam( pParam, isFlag, isLast );
} **/

bool CApplication::CCmdLineInfo::ParseSwitch( const TCHAR* pSwitch )
{
	ASSERT_PTR( pSwitch );

	if ( arg::Equals( pSwitch, _T("TEST") ) || arg::Equals( pSwitch, _T("UT") ) )
		SetFlag( m_pApp->m_runFlags, RunTests );
	else if ( arg::Equals( pSwitch, _T("SKIP_UI_TESTS") ) )
		SetFlag( m_pApp->m_runFlags, SkipUiTests );
	else
		switch ( func::ToUpper()( pSwitch[ 0 ] ) )
		{
			case _T('R'):	SetForceFlag( app::RegAdditionalExt, pSwitch[ 1 ] != _T('-') ); break;
			case _T('F'):	SetForceFlag( app::FullScreen, pSwitch[ 1 ] != _T('-') ); break;
			case _T('X'):	SetForceFlag( app::DocMaximize, pSwitch[ 1 ] != _T('-') ); break;
			case _T('T'):	SetForceFlag( app::ShowToolbar, pSwitch[ 1 ] != _T('-') ); break;
			case _T('S'):	SetForceFlag( app::ShowStatusBar, pSwitch[ 1 ] != _T('-') ); break;
			case _T('H'):
			case _T('?'):
				SetFlag( m_pApp->m_runFlags, ShowHelp );
				break;
			default:
				return false;
		}
	return true;
}

void CApplication::CCmdLineInfo::SetForceFlag( int forceFlag, bool on )
{
	SetFlag( m_pApp->m_forceMask, forceFlag, true );
	SetFlag( m_pApp->m_forceFlags, forceFlag, on );
}

std::tstring CApplication::CCmdLineInfo::GetHelpMsg( void )
{
	return str::FromAnsi(
		"Slider command line options:\n"
		"\n"
		"slider [/R[-]] [/F[-]] [/X[-]] [/T[-]] [/S[-]] [/?] [image_file] [image_folder]\n"
		"\n"
		"    /R   Register slider additional shell extensions.\n"
		"    /F   Open slider window in full screen mode.\n"
		"    /X   Open slider window maximized.\n"
		"    /T   Open slider window with toolbar displayed.\n"
		"    /S   Open slider window with status-bar displayed.\n"
		"    /?   Display this help message.\n"
		"\n"
		"    image_file     One or many image file(s).\n"
		"    image_folder   Folder to search images for.\n"
		"\n"
		"    Note:\tIf [-] suffix is specified, than the specified option is switched off.\n"
#ifdef _DEBUG
		"\n"
		"Debug build options:\n"
		"    -TEST            Run unit tests and exit.\n"
		"    -SKIP_UI_TESTS   Skip executing unit test that use the test window.\n"
#endif
	);
}
