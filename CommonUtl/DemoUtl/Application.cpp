
#include "pch.h"
#include "Application.h"
#include "MainFrame.h"
#include "ChildFrame.h"
#include "ImageDialog.h"
#include "FileListDialog.h"
#include "TestTaskDialog.h"
#include "TestDoc.h"
#include "TestFormView.h"
#include "resource.h"
#include "test/ImageTests.h"
#include "utl/EnumTags.h"
#include "utl/StringUtilities.h"
#include "utl/UI/AboutBox.h"
#include "utl/UI/GpUtilities.h"
#include "utl/UI/ImageStore.h"
#include "utl/UI/LayoutEngine.h"
#include "utl/UI/VisualTheme.h"
#include "utl/test/ThreadingTests.hxx"		// include only in this test project to avoid the link dependency on Boost libraries in regular projects

#include "utl/UI/ContextMenuMgr.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#include "utl/UI/BaseApp.hxx"
#include "utl/UI/BaseFrameWnd.hxx"


CBaseWinAppEx::CBaseWinAppEx( void )
	: CBaseApp<CWinAppEx>()
{
}

CBaseWinAppEx::~CBaseWinAppEx()
{
}

bool CBaseWinAppEx::InitContextMenuMgr( void )
{
	// superseeds CWinAppEx::InitContextMenuManager()
	if ( afxContextMenuManager != NULL )
	{
		ASSERT( false );		// already initialized
		return false;
	}

	afxContextMenuManager = new mfc::CContextMenuMgr();		// replace base singleton CContextMenuManager with ui::CContextMenuMgr, that has custom functionality
	m_bContextMenuManagerAutocreated = true;
	return true;
}


namespace reg
{
	static const TCHAR section[] = _T("Settings");
	static const TCHAR entry_disableSmooth[] = _T("DisableSmooth");
	static const TCHAR entry_disableThemes[] = _T("DisableThemes");
}


static const CImageStore::CCmdAlias s_cmdAliases[] =
{
	{ IDC_CLEAR_FILES_BUTTON, ID_REMOVE_ALL_ITEMS },
	{ IDC_COPY_SOURCE_PATHS_BUTTON, ID_EDIT_COPY },
	{ IDC_PASTE_FILES_BUTTON, ID_EDIT_PASTE },
	{ ID_NUMERIC_SEQUENCE_2DIGITS, ID_SHUTTLE_TOP },
	{ ID_NUMERIC_SEQUENCE_3DIGITS, ID_SHUTTLE_BOTTOM },
	{ ID_NUMERIC_SEQUENCE_4DIGITS, ID_RECORD_NEXT },
	{ ID_NUMERIC_SEQUENCE_5DIGITS, ID_RECORD_PREV }
};

const CEnumTags& GetTags_ResizeStyle( void )
{
	static const CEnumTags tags( _T("&Dialog...|H-&Dialog...|V-&Dialog...|Max &Dialog...") );
	return tags;
}

const CEnumTags& GetTags_ChangeCase( void )
{
	static const CEnumTags tags( str::Load( IDS_CHANGE_CASE_TAGS ) );
	return tags;
}


namespace hlp
{
	void CheckScalarTypes( void );
}


// CApplication implementation

CApplication g_theApp;	// the one and only CApplication object

CApplication::CApplication( void )
{
	SetInteractive( false );		// using an app message loop
}

CApplication::~CApplication()
{
}

BOOL CApplication::InitInstance( void )
{
	m_pGdiPlusInit.reset( new CScopedGdiPlusInit() );

	// init MFC control bars:
	InitContextMenuMgr();
	StoreVisualManagerClass( RUNTIME_CLASS( CMFCVisualManagerOffice2007 ) );

	if ( !CBaseWinAppEx::InitInstance() )
		return FALSE;

	SetRegistryBase( _T("Settings") );

	GetSharedImageStore()->RegisterToolbarImages( IDR_IMAGE_STRIP );
	GetSharedImageStore()->RegisterToolbarImages( IDR_LOW_COLOR_STRIP, color::Magenta );		// low color images
	GetSharedImageStore()->RegisterAliases( ARRAY_SPAN( s_cmdAliases ) );

	CAboutBox::s_appIconId = IDR_MAINFRAME;
	CLayoutEngine::m_defaultFlags = GetProfileInt( reg::section, reg::entry_disableSmooth, FALSE ) ? CLayoutEngine::Normal : CLayoutEngine::Smooth;
	CVisualTheme::SetEnabled( !GetProfileInt( reg::section, reg::entry_disableThemes, FALSE ) );

	//hlp::CheckScalarTypes();
	if ( HasCommandLineOptions() )
		return FALSE;					// no app loop

	LoadStdProfileSettings( 10 );  // Load standard INI file options (including MRU)
	// Register the application's document templates.  Document templates
	//  serve as the connection between documents, frame windows and views
	AddDocTemplate( new CMultiDocTemplate( IDR_TEST_DOC_TYPE,
		RUNTIME_CLASS( CTestDoc ),
		RUNTIME_CLASS( CChildFrame ),			// custom MDI child frame
		RUNTIME_CLASS( CTestFormView ) )
	);

//	m_nCmdShow = SW_SHOWMAXIMIZED;
	// create main MDI Frame window
	CMainFrame* pMainFrame = new CMainFrame();
	m_pMainWnd = pMainFrame;
	if ( !static_cast<CMainFrame*>( m_pMainWnd )->LoadFrame( IDR_MAINFRAME ) )
	{
		delete m_pMainWnd;
		return FALSE;
	}
	// call DragAcceptFiles only if there's a suffix
	//  In an MDI app, this should occur immediately after setting m_pMainWnd

#ifdef _DEBUG
	// register unit tests
//	CImageTests::Instance();		will be run on IDC_RUN_IMAGE_TESTS button
#endif

	// parse command line for standard shell commands, DDE, file open
	CCommandLineInfo cmdInfo;
	ParseCommandLine( cmdInfo );

	// dispatch commands specified on the command line; will return FALSE if app was launched with /RegServer, /Register, /Unregserver or /Unregister.
	if ( !ProcessShellCommand( cmdInfo ) )
		return FALSE;

	pMainFrame->ShowAppWindow( m_nCmdShow );
	m_pMainWnd->UpdateWindow();
	return TRUE;
}

int CApplication::ExitInstance( void )
{
	WriteProfileInt( reg::section, reg::entry_disableSmooth, !HasFlag( CLayoutEngine::m_defaultFlags, CLayoutEngine::SmoothGroups ) );
	WriteProfileInt( reg::section, reg::entry_disableThemes, CVisualTheme::IsDisabled() );

	return CBaseWinAppEx::ExitInstance();
}

void CApplication::OnInitAppResources( void )
{
	__super::OnInitAppResources();

#ifdef USE_UT
	// special case: in this demo project we include the threading tests, with their dependency on Boos Threads library
	CThreadingTests::Instance();
#endif
}

bool CApplication::HasCommandLineOptions( void )
{
	std::tstring imagePath;
	if ( app::HasCommandLineOption( _T("image"), &imagePath ) )		// "-image=<img_path>"
	{
		CImageDialog dlg( nullptr );
		m_pMainWnd = &dlg;
		if ( !imagePath.empty() )
			dlg.SetImagePath( imagePath );
		dlg.DoModal();
		return true;
	}
	else if ( app::HasCommandLineOption( _T("diffs") ) )			// "-diffs"
	{
		CFileListDialog dlg( nullptr );
		m_pMainWnd = &dlg;
		dlg.DoModal();
		return true;
	}
	else if ( app::HasCommandLineOption( _T("td") ) )			// "-td"
	{
		CTestTaskDialog dlg( nullptr );
		m_pMainWnd = &dlg;
		dlg.DoModal();
		return true;
	}
	else if ( app::HasCommandLineOption( _T("ut") ) )				// "-ut"
	{
		OnRunUnitTests();
		return true;
	}

	return false;
}


BEGIN_MESSAGE_MAP( CApplication, CBaseWinAppEx )
	ON_COMMAND( ID_FILE_NEW, &CBaseWinAppEx::OnFileNew )
	ON_COMMAND( ID_FILE_OPEN, &CBaseWinAppEx::OnFileOpen )
END_MESSAGE_MAP()


namespace hlp
{
	#define TRACE_SCALAR( type ) \
		TRACE( _T("sizeof( %s ) = %d bytes\n"), _T(#type), sizeof( type ) );

	void CheckScalarTypes( void )
	{
		TRACE( _T("*** CheckScalarTypes for %d bit build ***\n"), sizeof( void* ) * 8 );

		TRACE_SCALAR( bool );
		TRACE_SCALAR( char );
		TRACE_SCALAR( short );
		TRACE_SCALAR( int );
		TRACE_SCALAR( long );
		TRACE_SCALAR( __int64 );
		TRACE_SCALAR( long long );

		TRACE_SCALAR( size_t );
		TRACE_SCALAR( DWORD );
		TRACE_SCALAR( DWORD_PTR );

		TRACE_SCALAR( float );
		TRACE_SCALAR( double );
	}
}
