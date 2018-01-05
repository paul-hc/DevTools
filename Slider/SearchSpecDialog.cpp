
#include "StdAfx.h"
#include "SearchSpecDialog.h"
#include "Application.h"
#include "resource.h"
#include "utl/Path.h"
#include "utl/Utilities.h"
#include <io.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace reg
{
	const TCHAR section_search[] = _T("Search");
	const TCHAR entry_FolderHist[] = _T("FolderHistory");
	const TCHAR entry_FilterHist[] = _T("FilterHist");
}

CSearchSpecDialog::CSearchSpecDialog( const CSearchSpec& searchSpec, CWnd* pParent /*=NULL*/ )
	: CDialog( IDD_SEARCH_SPEC_DIALOG, pParent )
	, m_searchSpec( searchSpec )
	, m_searchPathCombo( ui::HistoryMaxSize, PROF_SEP )
	, m_searchFiltersCombo( ui::HistoryMaxSize, PROF_SEP )
{
}

CSearchSpecDialog::~CSearchSpecDialog()
{
}

void CSearchSpecDialog::DoDataExchange( CDataExchange* pDX )
{
	bool firstInit = NULL == m_searchPathCombo.m_hWnd;

	DDX_Control( pDX, IDC_SEARCH_FOLDER_COMBO, m_searchPathCombo );
	DDX_Control( pDX, IDC_SEARCH_FILTERS_COMBO, m_searchFiltersCombo );
	ui::DDX_EnumCombo( pDX, IDC_SEARCH_OPTIONS_COMBO, m_searchSpec.m_options );

	if ( !pDX->m_bSaveAndValidate )
	{
		if ( firstInit )
		{
			m_searchPathCombo.LimitText( _MAX_PATH );
			m_searchFiltersCombo.LimitText( _MAX_PATH );

			m_searchPathCombo.LoadHistory( reg::section_search, reg::entry_FolderHist );
			m_searchFiltersCombo.LoadHistory( reg::section_search, reg::entry_FilterHist );
		}
	}

	ui::DDX_Path( pDX, IDC_SEARCH_FOLDER_COMBO, m_searchSpec.m_searchPath );
	ui::DDX_Text( pDX, IDC_SEARCH_FILTERS_COMBO, m_searchSpec.m_searchFilters, true );

	if ( pDX->m_bSaveAndValidate )
	{	// setup the search attributes from itself
		m_searchSpec.Setup( m_searchSpec.m_searchPath, m_searchSpec.m_searchFilters, m_searchSpec.m_options );

		if ( m_searchSpec.IsValidPath() )
			m_searchPathCombo.SaveHistory( reg::section_search, reg::entry_FolderHist );
		m_searchFiltersCombo.SaveHistory( reg::section_search, reg::entry_FilterHist );
	}

	CDialog::DoDataExchange( pDX );
}

bool CSearchSpecDialog::ValidateOK( ui::ComboField byField /*= BySel*/ )
{
	std::tstring searchPath = ui::GetComboSelText( m_searchPathCombo, byField );

	bool validPath = path::IsValid( searchPath );
	bool isDirPath = validPath && fs::IsValidDirectory( searchPath.c_str() );

	ui::EnableControl( m_hWnd, IDOK, validPath );
	ui::EnableControl( m_hWnd, IDC_SEARCH_FILTERS_COMBO, isDirPath );
	ui::EnableControl( m_hWnd, IDC_SEARCH_OPTIONS_COMBO, isDirPath );
	return !searchPath.empty();
}


// message handlers

BEGIN_MESSAGE_MAP( CSearchSpecDialog, CDialog )
	ON_WM_DROPFILES()
	ON_BN_CLICKED( IDC_BROWSE_FOLDER_BUTTON, CmBrowseFolder )
	ON_CBN_EDITCHANGE( IDC_SEARCH_FOLDER_COMBO, OnCBnEditChangeSearchFolder )
	ON_CBN_SELCHANGE( IDC_SEARCH_FOLDER_COMBO, OnCBnSelChangeSearchFolder )
	ON_BN_CLICKED( IDC_BROWSE_FILE_BUTTON, CmBrowseFileButton )
END_MESSAGE_MAP()

BOOL CSearchSpecDialog::OnInitDialog( void )
{
	CDialog::OnInitDialog();
	DragAcceptFiles();

	ValidateOK();
	return TRUE;
}

void CSearchSpecDialog::OnDropFiles( HDROP hDropInfo )
{
	UINT fileCount = ::DragQueryFile( hDropInfo, (UINT)-1, NULL, 0 );
	if ( fileCount >= 1 )
	{	// handle just the first dropped directory/file
		TCHAR filePath[ _MAX_PATH ];
		::DragQueryFile( hDropInfo, 0, filePath, _MAX_PATH );
		ui::SetComboEditText( m_searchPathCombo, filePath );
		ValidateOK();
	}
	::DragFinish( hDropInfo );
}

void CSearchSpecDialog::CmBrowseFolder( void )
{
	CSearchSpec editSearchSpec( m_searchPathCombo.GetCurrentText() );
	if ( editSearchSpec.BrowseFilePath( CSearchSpec::PT_DirPath, this ) )
	{
		ui::SetComboEditText( m_searchPathCombo, editSearchSpec.m_searchPath.Get() );
		GotoDlgCtrl( &m_searchPathCombo );
		ValidateOK();
	}
}

void CSearchSpecDialog::CmBrowseFileButton( void )
{
	CSearchSpec editSearchSpec( m_searchPathCombo.GetCurrentText() );
	if ( editSearchSpec.BrowseFilePath( CSearchSpec::PT_FilePath, this ) )
	{
		ui::SetComboEditText( m_searchPathCombo, editSearchSpec.m_searchPath.Get() );
		GotoDlgCtrl( &m_searchPathCombo );
		ValidateOK();
	}
}

void CSearchSpecDialog::OnCBnEditChangeSearchFolder( void )
{
	ValidateOK( ui::ByEdit );
}

void CSearchSpecDialog::OnCBnSelChangeSearchFolder( void )
{
	ValidateOK( ui::BySel );
}
