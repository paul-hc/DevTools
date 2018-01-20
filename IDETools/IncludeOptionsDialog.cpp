
#include "stdafx.h"
#include "IncludeOptionsDialog.h"
#include "IncludeOptions.h"
#include "IncludePaths.h"
#include "ModuleSession.h"
#include "Application.h"
#include "utl/LayoutEngine.h"
#include "utl/Utilities.h"
#include "utl/resource.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace layout
{
	static CLayoutStyle styles[] =
	{
		{ IDOK, MoveX },
		{ IDCANCEL, MoveX },
		{ IDC_EXCLUDED_FN_EDIT, SizeX },
		{ IDC_MORE_FN_EDIT, SizeX },
		{ IDC_ADDITIONAL_INC_PATH_EDIT, SizeX }
	};
}


CIncludeOptionsDialog::CIncludeOptionsDialog( CIncludeOptions* pOptions, CWnd* pParent )
	: CLayoutDialog( IDD_INCLUDE_OPTIONS_DIALOG, pParent )
	, m_pOptions( pOptions )
{
	ASSERT_PTR( m_pOptions );
	m_regSection = _T("IncludeOptionsDialog");
	RegisterCtrlLayout( layout::styles, COUNT_OF( layout::styles ) );
	GetLayoutEngine().MaxClientSize().cy = 0;
	LoadDlgIcon( ID_OPTIONS );

	m_maxParseLinesEdit.SetValidRange( Range< int >( 5, 50000 ) );
	m_excludedEdit.SetContentType( ui::FilePath );
	m_includedEdit.SetContentType( ui::FilePath );
	m_additionalIncPathEdit.SetContentType( ui::DirPath );
}

void CIncludeOptionsDialog::DoDataExchange( CDataExchange* pDX )
{
	bool firstInit = NULL == m_depthLevelCombo.m_hWnd;

	DDX_Control( pDX, IDC_DEPTH_LEVEL_COMBO, m_depthLevelCombo );
	DDX_Control( pDX, IDC_MAX_PARSED_LINES_EDIT, m_maxParseLinesEdit );
	DDX_Control( pDX, IDC_EXCLUDED_FN_EDIT, m_excludedEdit );
	DDX_Control( pDX, IDC_MORE_FN_EDIT, m_includedEdit );
	DDX_Control( pDX, IDC_ADDITIONAL_INC_PATH_EDIT, m_additionalIncPathEdit );

	if ( firstInit )
		ui::WriteComboItems( m_depthLevelCombo, CIncludeOptions::GetTags_DepthLevel().GetUiTags() );

	DDX_Text( pDX, IDC_MAX_PARSED_LINES_EDIT, m_pOptions->m_maxParseLines );
	ui::DDX_Bool( pDX, IDC_REMOVE_DUPLICATES_CHECK, m_pOptions->m_noDuplicates );
	ui::DDX_Bool( pDX, IDC_OPEN_BLOWN_UP_CHECK, m_pOptions->m_openBlownUp );
	ui::DDX_Bool( pDX, IDC_SELECTION_RECOVER_CHECK, m_pOptions->m_selRecover );
	ui::DDX_Bool( pDX, IDC_LAZY_PARSING_CHECK, m_pOptions->m_lazyParsing );

	if ( !m_pOptions->m_lazyParsing )
		DDX_CBIndex( pDX, IDC_DEPTH_LEVEL_COMBO, m_pOptions->m_maxNestingLevel );

	if ( DialogOutput == pDX->m_bSaveAndValidate )
	{
		// change UI separators from PROF_SEP to EDIT_SEP
		std::tstring fnExclude = m_pOptions->m_fnExclude;
		str::Replace( fnExclude, PROF_SEP, EDIT_SEP );
		ui::SetWindowText( m_excludedEdit, fnExclude );

		std::tstring fnMore = m_pOptions->m_fnMore;
		str::Replace( fnMore, PROF_SEP, EDIT_SEP );
		ui::SetWindowText( m_includedEdit, fnMore );

		ui::SetWindowText( m_additionalIncPathEdit, app::GetModuleSession().GetAdditionalIncludePath() );

		CkDelayedParsing();
	}
	else
	{
		m_pOptions->SetupArrayExclude( ui::GetWindowText( m_excludedEdit ).c_str(), EDIT_SEP );
		m_pOptions->SetupArrayMore( ui::GetWindowText( m_includedEdit ).c_str(), EDIT_SEP );
		app::GetIncludePaths().SetMoreAdditionalIncludePath( ui::GetWindowText( m_additionalIncPathEdit ) );
	}

	CLayoutDialog::DoDataExchange( pDX );
}


// message handlers

BEGIN_MESSAGE_MAP( CIncludeOptionsDialog, CLayoutDialog )
	ON_BN_CLICKED( IDC_LAZY_PARSING_CHECK, CkDelayedParsing )
END_MESSAGE_MAP()

void CIncludeOptionsDialog::CkDelayedParsing( void )
{
	bool isDelayedParsing = IsDlgButtonChecked( IDC_LAZY_PARSING_CHECK ) != FALSE;
	int indexToSelect = isDelayedParsing ? 0 : m_pOptions->m_maxNestingLevel;

	m_depthLevelCombo.EnableWindow( !isDelayedParsing );
	m_depthLevelCombo.SetCurSel( indexToSelect );
}
