
#include "StdAfx.h"
#include "Application.h"
#include "DocTemplates.h"
#include "MainFrame.h"
#include "Workspace.h"
#include "AlbumDoc.h"
#include "ImageArchiveStg.h"
#include "MoveFileDialog.h"
#include "OleImagesDataSource.h"
#include "UT/ImagingD2DTests.h"
#include "UT/ThumbnailTests.h"
#include "resource.h"
#include "utl/AboutBox.h"
#include "utl/DragListCtrl.h"
#include "utl/GdiPlus_fwd.h"
#include "utl/RuntimeException.h"
#include "utl/UtilitiesEx.h"
#include "utl/Thumbnailer.h"
#include "utl/WicImageCache.h"
#include "utl/ut/WicImageTests.h"
#include <io.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#include "utl/BaseApp.hxx"


static const CImageStore::CCmdAlias cmdAliases[] =
{
	{ CM_OPEN_IMAGE_FILE, ID_FILE_OPEN },
	{ CM_DELETE_FILE, ID_REMOVE_ITEM },
	{ CM_EDIT_ALBUM, ID_EDIT_ITEM },
	{ CM_REFRESH_CONTENT, ID_REFRESH },
	{ ID_FILE_OPEN_ALBUM_FOLDER, ID_BROWSE_FOLDER },
	{ CM_CUSTOM_ORDER_UNDO, ID_EDIT_UNDO },
	{ CM_CUSTOM_ORDER_REDO, ID_EDIT_REDO },

	// * special case: toolbar buttons that use controls must have a placeholder image associated; otherwise the image list gets shifted completely
	{ IDW_AUTO_IMAGE_SIZE_COMBO, ID_EDIT_DETAILS },
	{ IDW_ZOOM_COMBO, ID_EDIT_DETAILS },
	{ IDW_NAV_SLIDER, ID_EDIT_DETAILS }
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

		TCHAR messageBuffer[ 512 ];
		ASSERT_PTR( pExc );
		pExc->GetErrorMessage( messageBuffer, COUNT_OF( messageBuffer ) );

		std::tstring message = str::Format( _T("* C++ MFC exception: %s"), messageBuffer );
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
		static ui::CIssueStore issueStore;
		return issueStore;
	}

	bool IsImageArchiveDoc( const TCHAR* pFilePath )
	{
		return CImageArchiveStg::HasImageArchiveExt( pFilePath );
	}

	std::tstring FormatSliderVersion( SliderVersion version )
	{
		return str::Format( _T("Slider v%x.%x"), ( version & 0xF0 ) >> 4, version & 0x0F );
	}

	bool MoveFiles( const std::vector< std::tstring >& filesToMove )
	{
		CMoveFileDialog moveToDialog( filesToMove, AfxGetMainWnd() );
		if ( moveToDialog.DoModal() != IDOK )
			return false;

		CTempCloneFileSet tempClone( filesToMove );		// make temporary copies for logical files (if any)
		std::vector< std::tstring > srcPhysicalPaths;
		str::cvt::MakeItemsAs( srcPhysicalPaths, tempClone.GetPhysicalFilePaths() );

		return shell::MoveFiles( srcPhysicalPaths, moveToDialog.m_destFolderPath.Get(), AfxGetMainWnd() );
	}

	bool DeleteFiles( const std::vector< std::tstring >& filesToDelete, bool allowUndo /*= true*/ )
	{
		return shell::DeleteFiles( filesToDelete, AfxGetMainWnd(), allowUndo ? FOF_ALLOWUNDO : 0 );
	}



	// CScopedUserReport implementation

	ui::IUserReport* CScopedUserReport::s_pUserReport = &app::CInteractiveMode::Instance();


	// CInteractiveMode implementation

	ui::IUserReport& CInteractiveMode::Instance( void )
	{
		static CInteractiveMode interactiveMode;
		return interactiveMode;
	}

	int CInteractiveMode::ReportError( CException* pExc, UINT mbType /*= MB_OK*/ )
	{
		return HandleReportException( pExc, mbType );
	}


	// CScopedProgress implementation

	CScopedProgress::CScopedProgress( int autoClearDelay /*= 250*/ )
		: m_pSharedProgressBar( app::GetMainFrame()->GetProgressCtrl() )
		, m_autoClearDelay( autoClearDelay )
		, m_pbStepIndex( 0 )
		, m_pbStepDivider( 1 )
	{
	}

	CScopedProgress::CScopedProgress( int valueMin, int count, int stepCount, const TCHAR* pCaption /*= NULL*/, int autoClearDelay /*= 250*/ )
		: m_pSharedProgressBar( app::GetMainFrame()->GetProgressCtrl() )
		, m_autoClearDelay( autoClearDelay )
		, m_pbStepIndex( 0 )
		, m_pbStepDivider( 1 )
		, m_pMessagePump( new CScopedPumpMessage( 5, CWnd::GetActiveWindow() ) )
	{
		Begin( valueMin, count, stepCount, pCaption );
	}

	CScopedProgress::~CScopedProgress()
	{
		if ( m_autoClearDelay != ACD_NoClear )
			End();
	}

	bool CScopedProgress::IsActive( void ) const
	{
		return app::GetMainFrame()->InProgress();
	}

	void CScopedProgress::Begin( int valueMin, int count, int stepCount, const TCHAR* pCaption /*= NULL*/ )
	{
		app::GetMainFrame()->BeginProgress( valueMin, count, stepCount, pCaption );
	}

	void CScopedProgress::End( int clearDelay /*= ACD_NoClear*/ )
	{
		app::GetMainFrame()->EndProgress( clearDelay == ACD_NoClear ? m_autoClearDelay : clearDelay );
	}

	void CScopedProgress::SetPos( int value )
	{
		app::GetMainFrame()->SetPosProgress( value );
	}

	void CScopedProgress::StepIt( void )
	{
		// if m_pbStepDivider > 1 divide StepIt calls by m_pbStepDivider
		if ( m_pbStepDivider <= 1 || !( ++m_pbStepIndex % m_pbStepDivider ) )
			app::GetMainFrame()->StepItProgress();

		if ( m_pMessagePump.get() != NULL )
			m_pMessagePump->CheckPump();
	}

	void CScopedProgress::GotoBegin( void )
	{
		int valueMin, valueMax;
		ASSERT( IsActive() );
		m_pSharedProgressBar->GetRange( valueMin, valueMax );
		m_pSharedProgressBar->SetPos( valueMin );
	}

	void CScopedProgress::GotoEnd( void )
	{
		int valueMin, valueMax;
		ASSERT( IsActive() );
		m_pSharedProgressBar->GetRange( valueMin, valueMax );
		m_pSharedProgressBar->SetPos( valueMax - 1 );
	}

} //namespace app


namespace app
{
	const std::tstring& GetAllSourcesSpecs( void )
	{
		static const std::tstring imgSpecs = CSliderFilters::Instance().MakeSpecs( shell::FileOpen );
		return imgSpecs;
	}

	bool BrowseArchiveStgFile( std::tstring& rFullPath, CWnd* pParentWnd, shell::BrowseMode browseMode /*= shell::FileOpen*/, DWORD flags /*= 0*/ )
	{
		static const std::tstring stgFilters = CAlbumFilterStore::Instance().MakeArchiveStgFilters();
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
	, m_pEventLogger( new CLogger( _T("%s-events") ) )
	, m_forceMask( 0 )
	, m_forceFlags( 0 )
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
	m_pGdiPlusInit.reset( new CScopedGdiPlusInit );

	if ( !CBaseApp< CWinApp >::InitInstance() )
		return FALSE;

	LoadStdProfileSettings( 10 );		// load standard INI file options (including MRU)

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
	GetSharedResources().AddAutoClear( &CImageArchiveStg::Factory() );
	GetSharedResources().AddAutoClear( &CWicImageCache::Instance() );
	m_pThumbnailer->SetExternalProducer( &CImageArchiveStg::Factory() );		// add as producer of storage-based thumbnails

	CAboutBox::m_appIconId = IDR_MAINFRAME;
	m_sharedAccel.Load( IDR_COMMAND_BAR_ACCEL );
	CToolStrip::RegisterStripButtons( IDR_MAINFRAME );
	CImageStore::SharedStore()->RegisterAliases( cmdAliases, COUNT_OF( cmdAliases ) );

	ASSERT_NULL( m_pDocManager );
	m_pDocManager = new app::CDocManager;	// register document templates

	EnableShellOpen();						// enable DDE Execute open
	RegisterShellFileTypes( TRUE );			// register album extensions for Album.Document

	CCommandLineInfo cmdInfo;
	ParseCommandLine( cmdInfo );			// parse command line for standard shell commands, DDE, file open

	if ( HasForceMask( app::ShowHelp ) && HasForceFlag( app::ShowHelp ) )
	{
		AfxMessageBox( IDS_HELP_MESSAGE );
		return FALSE;
	}

	if ( ui::IsKeyPressed( VK_SHIFT ) )
		SetForceFlag( app::FullScreen | app::DocMaximize, true );

	// create main MDI Frame window
	m_pMainFrame = new CMainFrame;
	m_pMainWnd = m_pMainFrame;
	if ( !m_pMainFrame->LoadFrame( IDR_MAINFRAME ) )
		return FALSE;

	// check and adjust application's forced flags
	CWorkspace::Instance().AdjustForcedBehaviour();

	// adjust the MRU list max count
	delete m_pRecentFileList;
	m_pRecentFileList = NULL;

	LoadStdProfileSettings( CWorkspace::GetData().m_mruCount );

	// dispatch commands specified on the command line (if any):
	if ( !cmdInfo.m_strFileName.IsEmpty() )
		if ( !ProcessShellCommand( cmdInfo ) )
			return FALSE;

	// the main window has been initialized, so show and update it
	m_pMainFrame->ShowWindow( m_nCmdShow );
	m_pMainFrame->UpdateWindow();


#ifdef _DEBUG
	// register UTL tests
	CWicImageTests::Instance();
	CThumbnailTests::Instance();
	CImagingD2DTests::Instance();
#endif

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

void CApplication::ParseCommandLine( CCommandLineInfo& rCmdInfo )
{
	for ( int i = 1; i < __argc; i++ )
	{
		const TCHAR* pParam = __targv[ i ];
		bool isFlag = _T('-') == pParam[ 0 ] || _T('/') == pParam[ 0 ];

		if ( isFlag )
			++pParam;

		rCmdInfo.ParseParam( pParam, isFlag, __argc == ( i + 1 ) );

		if ( isFlag )
			switch ( _totlower( pParam[ 0 ] ) )
			{
				case _T('r'):	checked_static_cast< app::CDocManager* >( m_pDocManager )->RegisterImageAdditionalShellExt( pParam[ 1 ] != _T('-') ); break;
				case _T('f'):	SetForceFlag( app::FullScreen, pParam[ 1 ] != _T('-') ); break;
				case _T('x'):	SetForceFlag( app::DocMaximize, pParam[ 1 ] != _T('-') ); break;
				case _T('t'):	SetForceFlag( app::ShowToolbar, pParam[ 1 ] != _T('-') ); break;
				case _T('s'):	SetForceFlag( app::ShowStatusBar, pParam[ 1 ] != _T('-') ); break;
				case _T('h'):
				case _T('?'):	SetForceFlag( app::ShowHelp, pParam[ 1 ] != _T('-') ); break;
			}
	}
}

void CApplication::SetForceFlag( int forceFlag, bool on )
{
	SetFlag( m_forceMask, forceFlag, true );
	SetFlag( m_forceFlags, forceFlag, on );
}

bool CApplication::OpenQueuedAlbum( void )
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

void CApplication::SetStatusBarMessage( const TCHAR* pMessage )
{
	m_pMainFrame->SetMessageText( pMessage );
}

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
	CString command = pCommand;
	if ( command.Left( 8 ) == _T("[queue(\"") )
	{	// process each DDE "queue" request for explicit albums
		int trailingQuotePos = command.ReverseFind( _T('\"') );
		if ( -1 == trailingQuotePos )
			return FALSE;

		m_queuedAlbumFilePaths.push_back( command.Mid( 8, trailingQuotePos - 8 ).GetString() );
		app::GetMainFrame()->StartQueuedAlbumTimer();
		return TRUE;
	}
	else
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
	static std::tstring folderPath;
	if ( shell::BrowseForFolder( folderPath, AfxGetMainWnd(), NULL, shell::BF_FileSystem, _T("Browse Folder Images"), false ) )
		OpenDocumentFile( folderPath.c_str() );
}

void CApplication::OnClearTempEmbeddedClones( void )
{
	CTempCloneFileSet::ClearAllTempFiles();
}
