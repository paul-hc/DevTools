
#include "stdafx.h"
#include "SearchPatternDialog.h"
#include "resource.h"
#include "utl/Path.h"
#include "utl/UI/Utilities.h"
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

CSearchPatternDialog::CSearchPatternDialog( const CSearchPattern& searchPattern, CWnd* pParent /*=NULL*/ )
	: CDialog( IDD_SEARCH_PATTERN_DIALOG, pParent )
	, m_searchPattern( searchPattern )
	, m_searchPathCombo( ui::HistoryMaxSize, PROF_SEP )
	, m_wildFiltersCombo( ui::HistoryMaxSize, PROF_SEP )
{
}

CSearchPatternDialog::~CSearchPatternDialog()
{
}

void CSearchPatternDialog::DoDataExchange( CDataExchange* pDX )
{
	bool firstInit = NULL == m_searchPathCombo.m_hWnd;

	DDX_Control( pDX, IDC_SEARCH_FOLDER_COMBO, m_searchPathCombo );
	DDX_Control( pDX, IDC_SEARCH_FILTERS_COMBO, m_wildFiltersCombo );
	ui::DDX_EnumSelValue( pDX, IDC_SEARCH_OPTIONS_COMBO, m_searchPattern.RefSearchMode() );

	if ( DialogOutput == pDX->m_bSaveAndValidate )
	{
		if ( firstInit )
		{
			m_searchPathCombo.LimitText( _MAX_PATH );
			m_wildFiltersCombo.LimitText( _MAX_PATH );

			m_searchPathCombo.LoadHistory( reg::section_search, reg::entry_FolderHist );
			m_wildFiltersCombo.LoadHistory( reg::section_search, reg::entry_FilterHist );
		}
	}

	ui::DDX_PathItem( pDX, IDC_SEARCH_FOLDER_COMBO, &m_searchPattern );
	ui::DDX_Text( pDX, IDC_SEARCH_FILTERS_COMBO, m_searchPattern.RefWildFilters(), true );

	if ( DialogSaveChanges == pDX->m_bSaveAndValidate )
	{	// setup the search attributes from itself
		m_searchPattern.SetAutoType();

		if ( m_searchPattern.IsValidPath() )
			m_searchPathCombo.SaveHistory( reg::section_search, reg::entry_FolderHist );

		m_wildFiltersCombo.SaveHistory( reg::section_search, reg::entry_FilterHist );
	}

	CDialog::DoDataExchange( pDX );
}

bool CSearchPatternDialog::ValidateOK( ui::ComboField byField /*= BySel*/ )
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

BEGIN_MESSAGE_MAP( CSearchPatternDialog, CDialog )
	ON_WM_DROPFILES()
	ON_BN_CLICKED( IDC_BROWSE_FOLDER_BUTTON, CmBrowseFolder )
	ON_CBN_EDITCHANGE( IDC_SEARCH_FOLDER_COMBO, OnCBnEditChangeSearchFolder )
	ON_CBN_SELCHANGE( IDC_SEARCH_FOLDER_COMBO, OnCBnSelChangeSearchFolder )
	ON_BN_CLICKED( IDC_BROWSE_FILE_BUTTON, CmBrowseFileButton )
END_MESSAGE_MAP()

BOOL CSearchPatternDialog::OnInitDialog( void )
{
	CDialog::OnInitDialog();
	DragAcceptFiles();

	ValidateOK();
	return TRUE;
}

void CSearchPatternDialog::OnDropFiles( HDROP hDropInfo )
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

void CSearchPatternDialog::CmBrowseFolder( void )
{
	CSearchPattern editSearchPattern( m_searchPathCombo.GetCurrentText() );
	if ( editSearchPattern.BrowseFilePath( CSearchPattern::BrowseAsDirPath, this ) )
	{
		ui::SetComboEditText( m_searchPathCombo, editSearchPattern.GetFilePath().Get() );
		GotoDlgCtrl( &m_searchPathCombo );
		ValidateOK();
	}
}

void CSearchPatternDialog::CmBrowseFileButton( void )
{
	CSearchPattern editSearchPattern( m_searchPathCombo.GetCurrentText() );
	if ( editSearchPattern.BrowseFilePath( CSearchPattern::BrowseAsFilePath, this ) )
	{
		ui::SetComboEditText( m_searchPathCombo, editSearchPattern.GetFilePath().Get() );
		GotoDlgCtrl( &m_searchPathCombo );
		ValidateOK();
	}
}

void CSearchPatternDialog::OnCBnEditChangeSearchFolder( void )
{
	ValidateOK( ui::ByEdit );
}

void CSearchPatternDialog::OnCBnSelChangeSearchFolder( void )
{
	ValidateOK( ui::BySel );
}
