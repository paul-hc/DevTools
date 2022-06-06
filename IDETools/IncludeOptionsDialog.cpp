
#include "stdafx.h"
#include "IncludeOptionsDialog.h"
#include "IncludeOptions.h"
#include "IncludePaths.h"
#include "ModuleSession.h"
#include "Application.h"
#include "utl/UI/LayoutEngine.h"
#include "utl/UI/WndUtils.h"
#include "utl/UI/resource.h"
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
		{ IDC_FN_IGNORED_EDIT, SizeX },
		{ IDC_FN_ADDED_EDIT, SizeX },
		{ IDC_ADDITIONAL_INC_PATH_EDIT, SizeX }
	};
}


CIncludeOptionsDialog::CIncludeOptionsDialog( CIncludeOptions* pOptions, CWnd* pParent )
	: CLayoutDialog( IDD_INCLUDE_OPTIONS_DIALOG, pParent )
	, m_pOptions( pOptions )
	, m_depthLevelCombo( &CIncludeOptions::GetTags_DepthLevel() )
{
	ASSERT_PTR( m_pOptions );
	m_regSection = _T("IncludeOptionsDialog");
	RegisterCtrlLayout( ARRAY_PAIR( layout::styles ) );
	GetLayoutEngine().DisableResizeVertically();
	LoadDlgIcon( ID_OPTIONS );

	m_maxParseLinesEdit.SetValidRange( Range<int>( 5, 50000 ) );
	m_ignoredEdit.SetContentType( ui::FilePath );
	m_addedEdit.SetContentType( ui::FilePath );
	m_additionalIncPathEdit.SetContentType( ui::DirPath );
}

void CIncludeOptionsDialog::DoDataExchange( CDataExchange* pDX )
{
	DDX_Control( pDX, IDC_DEPTH_LEVEL_COMBO, m_depthLevelCombo );
	DDX_Control( pDX, IDC_MAX_PARSED_LINES_EDIT, m_maxParseLinesEdit );
	DDX_Control( pDX, IDC_FN_IGNORED_EDIT, m_ignoredEdit );
	DDX_Control( pDX, IDC_FN_ADDED_EDIT, m_addedEdit );
	DDX_Control( pDX, IDC_ADDITIONAL_INC_PATH_EDIT, m_additionalIncPathEdit );

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
		ui::SetWindowText( m_ignoredEdit, m_pOptions->m_fnIgnored.Join( EDIT_SEP ) );
		ui::SetWindowText( m_addedEdit, m_pOptions->m_fnAdded.Join( EDIT_SEP ) );
		ui::SetWindowText( m_additionalIncPathEdit, m_pOptions->m_additionalIncludePath.Join( EDIT_SEP ) );

		OnToggle_DelayedParsing();
	}
	else
	{
		m_pOptions->m_fnIgnored.Split( ui::GetWindowText( m_ignoredEdit ).c_str(), EDIT_SEP );
		m_pOptions->m_fnAdded.Split( ui::GetWindowText( m_addedEdit ).c_str(), EDIT_SEP );
		m_pOptions->m_additionalIncludePath.Split( ui::GetWindowText( m_additionalIncPathEdit ).c_str(), EDIT_SEP );
	}

	CLayoutDialog::DoDataExchange( pDX );
}


// message handlers

BEGIN_MESSAGE_MAP( CIncludeOptionsDialog, CLayoutDialog )
	ON_BN_CLICKED( IDC_LAZY_PARSING_CHECK, OnToggle_DelayedParsing )
END_MESSAGE_MAP()

void CIncludeOptionsDialog::OnOK( void )
{
	m_pOptions->Save();
	CLayoutDialog::OnOK();
}

void CIncludeOptionsDialog::OnToggle_DelayedParsing( void )
{
	bool isDelayedParsing = IsDlgButtonChecked( IDC_LAZY_PARSING_CHECK ) != FALSE;
	int indexToSelect = isDelayedParsing ? 0 : m_pOptions->m_maxNestingLevel;

	m_depthLevelCombo.EnableWindow( !isDelayedParsing );
	m_depthLevelCombo.SetValue( indexToSelect );
}
