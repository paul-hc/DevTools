
#include "stdafx.h"
#include "Application.h"
#include "MainFrame.h"
#include "ChildFrame.h"
#include "ImageDialog.h"
#include "FileListDialog.h"
#include "TestTaskDialog.h"
#include "ImageTests.h"
#include "TestDoc.h"
#include "TestFormView.h"
#include "resource.h"
#include "utl/EnumTags.h"
#include "utl/StringUtilities.h"
#include "utl/UI/AboutBox.h"
#include "utl/UI/GpUtilities.h"
#include "utl/UI/ImageStore.h"
#include "utl/UI/LayoutEngine.h"
#include "utl/UI/VisualTheme.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#include "utl/UI/BaseApp.hxx"


namespace reg
{
	static const TCHAR section[] = _T("Settings");
	static const TCHAR entry_disableSmooth[] = _T("DisableSmooth");
	static const TCHAR entry_disableThemes[] = _T("DisableThemes");
}


static const CImageStore::CCmdAlias cmdAliases[] =
{
	{ IDC_CLEAR_FILES_BUTTON, ID_REMOVE_ALL_ITEMS },
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


// CApplication implementation

CApplication theApp;	// the one and only CApplication object

CApplication::CApplication( void )
{
}

CApplication::~CApplication()
{
}

BOOL CApplication::InitInstance( void )
{
	m_pGdiPlusInit.reset( new CScopedGdiPlusInit );

	if ( !CBaseApp< CWinApp >::InitInstance() )
		return FALSE;

	CAboutBox::m_appIconId = IDR_MAINFRAME;
	CToolStrip::RegisterStripButtons( IDR_IMAGE_STRIP );
	CToolStrip::RegisterStripButtons( IDR_LOW_COLOR_STRIP, color::Magenta );		// low color images
	CImageStore::SharedStore()->RegisterAliases( cmdAliases, COUNT_OF( cmdAliases ) );

	CLayoutEngine::m_defaultFlags = GetProfileInt( reg::section, reg::entry_disableSmooth, FALSE ) ? CLayoutEngine::Normal : CLayoutEngine::Smooth;
	CVisualTheme::SetEnabled( !GetProfileInt( reg::section, reg::entry_disableThemes, FALSE ) );

	std::tstring imagePath;
	if ( app::HasCommandLineOption( _T("image"), &imagePath ) )		// "-image=<img_path>"
	{
		CImageDialog dlg( NULL );
		m_pMainWnd = &dlg;
		if ( !imagePath.empty() )
			dlg.SetImagePath( imagePath );
		dlg.DoModal();
		return FALSE;					// no app loop
	}
	else if ( app::HasCommandLineOption( _T("diffs") ) )			// "-diffs"
	{
		CFileListDialog dlg( NULL );
		m_pMainWnd = &dlg;
		dlg.DoModal();
		return FALSE;					// no app loop
	}
	else if ( app::HasCommandLineOption( _T("td") ) )			// "-td"
	{
		CTestTaskDialog dlg( NULL );
		m_pMainWnd = &dlg;
		dlg.DoModal();
		return FALSE;					// no app loop
	}
	else if ( app::HasCommandLineOption( _T("ut") ) )				// "-ut"
	{
		OnRunUnitTests();
		return FALSE;					// no app msg loop
	}

	LoadStdProfileSettings( 4 );  // Load standard INI file options (including MRU)
	// Register the application's document templates.  Document templates
	//  serve as the connection between documents, frame windows and views
	AddDocTemplate( new CMultiDocTemplate( IDR_TEST_DOC_TYPE,
		RUNTIME_CLASS( CTestDoc ),
		RUNTIME_CLASS( CChildFrame ),			// custom MDI child frame
		RUNTIME_CLASS( CTestFormView ) )
	);

	m_nCmdShow = SW_SHOWMAXIMIZED;
	// create main MDI Frame window
	m_pMainWnd = new CMainFrame;
	if ( !static_cast< CMainFrame* >( m_pMainWnd )->LoadFrame( IDR_MAINFRAME ) )
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

	m_pMainWnd->ShowWindow( m_nCmdShow );
	m_pMainWnd->UpdateWindow();
	return TRUE;
}

int CApplication::ExitInstance( void )
{
	WriteProfileInt( reg::section, reg::entry_disableSmooth, !HasFlag( CLayoutEngine::m_defaultFlags, CLayoutEngine::SmoothGroups ) );
	WriteProfileInt( reg::section, reg::entry_disableThemes, CVisualTheme::IsDisabled() );

	return CBaseApp< CWinApp >::ExitInstance();
}

BEGIN_MESSAGE_MAP( CApplication, CBaseApp< CWinApp > )
	ON_COMMAND( ID_FILE_NEW, &CBaseApp< CWinApp >::OnFileNew )
	ON_COMMAND( ID_FILE_OPEN, &CBaseApp< CWinApp >::OnFileOpen )
END_MESSAGE_MAP()
