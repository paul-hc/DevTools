
#include "stdafx.h"
#include "SearchPatternDialog.h"
#include "resource.h"
#include "utl/Path.h"
#include "utl/UI/LayoutEngine.h"
#include "utl/UI/DialogToolBar.h"
#include "utl/UI/ShellUtilities.h"
#include "utl/UI/WndUtils.h"
#include "utl/UI/resource.h"
#include <io.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#include "utl/UI/TandemControls.hxx"


namespace reg
{
	const TCHAR section_search[] = _T("Search");
	const TCHAR entry_FolderHist[] = _T("FolderHistory");
	const TCHAR entry_FilterHist[] = _T("FilterHist");
}


namespace layout
{
	static const CLayoutStyle styles[] =
	{
		{ IDC_STRIP_BAR_1, MoveX },
		{ IDC_SEARCH_PATH_COMBO, SizeX },
		{ IDC_SEARCH_FILTERS_COMBO, SizeX },
		{ IDC_SEARCH_MODE_COMBO, SizeX },

		{ IDOK, MoveX },
		{ IDCANCEL, MoveX }
	};
}


CSearchPatternDialog::CSearchPatternDialog( const CSearchPattern* pSrcPattern, CWnd* pParent /*=NULL*/ )
	: CLayoutDialog( IDD_SEARCH_PATTERN_DIALOG, pParent )
	, m_pSearchPattern( pSrcPattern != NULL ? new CSearchPattern( *pSrcPattern ) : new CSearchPattern() )
	, m_searchPathCombo( ui::MixedPath )
	, m_wildFiltersCombo( ui::HistoryMaxSize, PROF_SEP )
	, m_searchModeCombo( &CSearchPattern::GetTags_SearchMode() )
{
	// base init
	m_regSection = _T("SearchPatternDialog");
	RegisterCtrlLayout( ARRAY_SPAN( layout::styles ) );
	GetLayoutEngine().DisableResizeVertically();

	ClearFlag( m_wildFiltersCombo.RefItemContent().m_itemsFlags, ui::CItemContent::RemoveEmpty );

	m_searchPathCombo.SetItemSep( PROF_SEP );
}

CSearchPatternDialog::~CSearchPatternDialog()
{
}

void CSearchPatternDialog::DoDataExchange( CDataExchange* pDX )
{
	bool firstInit = NULL == m_searchPathCombo.m_hWnd;

	m_searchPathCombo.DDX_Tandem( pDX, IDC_SEARCH_PATH_COMBO, this );
	DDX_Control( pDX, IDC_SEARCH_FILTERS_COMBO, m_wildFiltersCombo );
	m_searchModeCombo.DDX_EnumValue( pDX, IDC_SEARCH_MODE_COMBO, m_pSearchPattern->RefSearchMode() );

	if ( DialogOutput == pDX->m_bSaveAndValidate )
	{
		if ( firstInit )
		{
			DragAcceptFiles();

			m_searchPathCombo.LimitText( MAX_PATH );
			m_wildFiltersCombo.LimitText( MAX_PATH );

			m_searchPathCombo.LoadHistory( reg::section_search, reg::entry_FolderHist );
			m_wildFiltersCombo.LoadHistory( reg::section_search, reg::entry_FilterHist );
		}
	}

	ui::DDX_PathItem( pDX, IDC_SEARCH_PATH_COMBO, m_pSearchPattern.get() );
	ui::DDX_Text( pDX, IDC_SEARCH_FILTERS_COMBO, m_pSearchPattern->RefWildFilters(), true );

	if ( DialogSaveChanges == pDX->m_bSaveAndValidate )
	{	// setup the search attributes from itself
		m_pSearchPattern->SetAutoType();

		if ( m_pSearchPattern->IsValidPath() )
			m_searchPathCombo.SaveHistory( reg::section_search, reg::entry_FolderHist );

		m_wildFiltersCombo.SaveHistory( reg::section_search, reg::entry_FilterHist );
	}
	ValidatePattern();

	__super::DoDataExchange( pDX );
}

void CSearchPatternDialog::ValidatePattern( ui::ComboField byField /*= BySel*/ )
{
	fs::CPath searchPath = ui::GetComboSelText( m_searchPathCombo, byField );

	const CSearchPattern pattern( searchPath );
	bool validPattern = pattern.IsValidPath();
	bool isDirPath = pattern.IsDirPath();

	ui::EnableControl( m_hWnd, IDC_SEARCH_FILTERS_COMBO, isDirPath );
	ui::EnableControl( m_hWnd, IDC_SEARCH_MODE_COMBO, isDirPath );
	ui::EnableControl( m_hWnd, IDOK, validPattern );
}


// message handlers

BEGIN_MESSAGE_MAP( CSearchPatternDialog, CLayoutDialog )
	ON_WM_DROPFILES()
	ON_COMMAND( ID_BROWSE_FOLDER, OnBrowseFolder )
	ON_COMMAND( ID_BROWSE_FILE, OnBrowseFile )
	ON_UPDATE_COMMAND_UI_RANGE( ID_BROWSE_FILE, ID_BROWSE_FOLDER, OnUpdate_BrowsePath )
	ON_CBN_EDITCHANGE( IDC_SEARCH_PATH_COMBO, OnCBnEditChange_SearchFolder )
	ON_CBN_SELCHANGE( IDC_SEARCH_PATH_COMBO, OnCBnSelChange_SearchFolder )
	ON_CBN_EDITCHANGE( IDC_SEARCH_FILTERS_COMBO, OnCBnEditChange_WildFilters )
	ON_CBN_SELCHANGE( IDC_SEARCH_FILTERS_COMBO, OnCBnSelChange_WildFilters )
	ON_CBN_SELCHANGE( IDC_SEARCH_MODE_COMBO, OnCBnSelChange_SearchMode )
END_MESSAGE_MAP()

void CSearchPatternDialog::OnDropFiles( HDROP hDropInfo )
{
	std::vector< std::tstring > filePaths;
	shell::QueryDroppedFiles( filePaths, hDropInfo );

	if ( !filePaths.empty() )
	{
		m_pSearchPattern->SetFilePath( filePaths.front() );

		ui::SetComboEditText( m_searchPathCombo, filePaths.front() );
		ValidatePattern();
	}
}

void CSearchPatternDialog::OnBrowseFolder( void )
{
	UpdateData( DialogSaveChanges );

	if ( m_pSearchPattern->BrowseFilePath( CSearchPattern::BrowseAsDirPath, this ) )
	{
		UpdateData( DialogOutput );
		GotoDlgCtrl( &m_searchPathCombo );
	}
}

void CSearchPatternDialog::OnBrowseFile( void )
{
	UpdateData( DialogSaveChanges );

	if ( m_pSearchPattern->BrowseFilePath( CSearchPattern::BrowseAsFilePath, this ) )
	{
		UpdateData( DialogOutput );
		GotoDlgCtrl( &m_searchPathCombo );
	}
}

void CSearchPatternDialog::OnUpdate_BrowsePath( CCmdUI* pCmdUI )
{
	switch ( pCmdUI->m_nID )
	{
		case ID_BROWSE_FILE:
			pCmdUI->SetRadio( fs::IsValidFile( m_pSearchPattern->GetFilePath().GetPtr() ) );
			break;
		case ID_BROWSE_FOLDER:
			pCmdUI->SetRadio( fs::IsValidDirectory( m_pSearchPattern->GetFilePath().GetPtr() ) );
			break;
	}
}

void CSearchPatternDialog::OnCBnEditChange_SearchFolder( void )
{
	std::tstring newPath = ui::GetComboSelText( m_searchPathCombo, ui::ByEdit );

	if ( fs::FileExist( newPath.c_str() ) )
	{
		m_pSearchPattern->SetFilePath( newPath );
		UpdateData( DialogOutput );
	}
	else
	{	// input only file path (preserving the other fields)
		m_pSearchPattern->CPathItemBase::SetFilePath( newPath );
		ValidatePattern();
	}
}

void CSearchPatternDialog::OnCBnSelChange_SearchFolder( void )
{
	UpdateData( DialogSaveChanges );
	m_pSearchPattern->SetFilePath( m_searchPathCombo.GetCurrentText() );
	UpdateData( DialogOutput );
}

void CSearchPatternDialog::OnCBnEditChange_WildFilters( void )
{
	m_pSearchPattern->RefWildFilters() = ui::GetComboSelText( m_wildFiltersCombo, ui::ByEdit );
}

void CSearchPatternDialog::OnCBnSelChange_WildFilters( void )
{
	m_pSearchPattern->RefWildFilters() = m_wildFiltersCombo.GetCurrentText();
}

void CSearchPatternDialog::OnCBnSelChange_SearchMode( void )
{
	m_pSearchPattern->SetSearchMode( m_searchModeCombo.GetEnum< CSearchPattern::SearchMode >() );
}
