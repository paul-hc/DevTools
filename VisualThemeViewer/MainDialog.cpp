
#include "stdafx.h"
#include "MainDialog.h"
#include "Application.h"
#include "ThemeContext.h"
#include "ThemeCustomDraw.h"
#include "utl/Clipboard.h"
#include "utl/MenuUtilities.h"
#include "utl/StringUtilities.h"
#include "utl/Utilities.h"
#include "utl/StdColors.h"
#include "utl/SubjectPredicates.h"
#include "utl/resource.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace hlp
{
	std::tstring FormatCounts( unsigned int count, unsigned int total )
	{
		return str::Format( count != total ? _T("%d/%d") : _T("%d"), count, total );
	}
}


namespace reg
{
	static const TCHAR section[] = _T("MainDialog");
	static const TCHAR entry_selClass[] = _T("SelClass");
	static const TCHAR entry_selTreeItem[] = _T("SelTreeItem");
	static const TCHAR entry_bkColorHistory[] = _T("BkColorHistory");
	static const TCHAR entry_classFilter[] = _T("ClassFilter");
	static const TCHAR entry_partsFilter[] = _T("PartsFilter");
}


namespace layout
{
	enum { SizeSamplesPct = 25, MoveSamplesPct = SizeSamplesPct * 15 / 10 /* x1.5 */ };

	static CLayoutStyle layoutStyles[] =
	{
		{ IDC_CLASS_COMBO, DoRepaint },
		{ IDC_PARTS_AND_STATES_TREE, SizeY },
		{ IDC_SMALL_SAMPLE_STATIC, pctMoveX( MoveSamplesPct ) | pctSizeX( SizeSamplesPct ) },
		{ IDC_TINY_SAMPLE_STATIC, MoveX },
		{ IDC_MEDIUM_SAMPLE_STATIC, pctMoveX( MoveSamplesPct ) | pctSizeX( SizeSamplesPct ) },
		{ IDC_CORE_SIZE_STATIC, MoveX },
		{ IDC_LARGE_SAMPLE_LABEL, SizeY },
		{ IDC_LARGE_SAMPLE_STATIC, Size },
		{ IDC_OPTIONS_GROUP, MoveY | SizeX },
		{ IDC_BK_COLOR_STATIC, MoveY },
		{ IDC_BK_COLOR_COMBO, MoveY },
		{ IDC_USE_BORDER_CHECK, MoveY },
		{ IDC_PRE_BK_GUIDES_CHECK, MoveY },
		{ IDC_POST_BK_GUIDES_CHECK, MoveY },
		{ IDC_ENABLE_THEMES_CHECK, MoveY },
		{ IDC_ENABLE_THEMES_FALLBACK_CHECK, MoveY },
		{ IDCANCEL, Move }
	};
}


CMainDialog::CMainDialog( void )
	: CBaseMainDialog( IDD_MAIN_DIALOG )
	, m_options( this )
	, m_pCustomDraw( new CThemeCustomDraw( &m_options, _T("Text") ) )
	, m_internalChange( 0 )
{
	m_regSection = reg::section;
	RegisterCtrlLayout( layout::layoutStyles, COUNT_OF( layout::layoutStyles ) );
	m_themeStore.SetupNotImplementedThemes();				// mark not implemented themes as NotImplemented

	for ( int i = 0; i != SampleCount; ++i )
		m_samples[ i ].SetOptions( &m_options );

	m_samples[ Tiny ].m_coreSize = CSize( 10, 10 );
	m_samples[ Medium ].SetSizeToContentMode( IDC_CORE_SIZE_STATIC );
	m_samples[ Large ].m_sampleText = _T("Some Sample Themed Text");

	m_partStateTree.SetUseExplorerTheme( false );
	m_partStateTree.SetTextEffectCallback( this );
	m_partStateTree.SetTrackMenuTarget( this );
	m_partStateTree.GetCtrlAccel().Load( IDC_PARTS_AND_STATES_TREE );
	ui::LoadPopupMenu( m_partStateTree.GetContextMenu(), IDR_CONTEXT_MENU, app::ListTreePopup );
	m_partStateTree.SetCustomImageDraw( m_pCustomDraw.get(), CSize( 40, 24 ) );

	m_toolbar.GetStrip()
		.AddButton( ID_COPY_THEME )
		.AddButton( ID_COPY_THEME_PART_AND_STATE )
		.AddSeparator()
		.AddButton( ID_SHOW_THEME_GLYPHS_CHECK );
}

CMainDialog::~CMainDialog()
{
}

CWnd* CMainDialog::GetWnd( void )
{
	return this;
}

void CMainDialog::RedrawSamples( void )
{
	for ( int i = 0; i != SampleCount; ++i )
		m_samples[ i ].RedrawWindow( NULL, NULL );

	m_partStateTree.Invalidate();
}

void CMainDialog::CombineTextEffectAt( ui::CTextEffect& rTextEffect, LPARAM rowKey, int subItem ) const
{
	subItem;

	HTREEITEM hItem = reinterpret_cast< HTREEITEM >( rowKey );
	const IThemeNode* pThemeNode = m_partStateTree.GetItemObject< IThemeNode >( hItem );

	switch ( pThemeNode->GetRelevance() )
	{
		case MediumRelevance:
			rTextEffect.m_textColor = GetSysColor( COLOR_GRAYTEXT );
			break;
		case ObscureRelevance:
			rTextEffect.m_textColor = color::Grey25;
			break;
	}
}

void CMainDialog::SetupClassesCombo( void )
{
	std::tstring selClass = ui::GetComboSelText( m_classCombo );
	if ( selClass.empty() )
		selClass = AfxGetApp()->GetProfileString( reg::section, reg::entry_selClass );

	Relevance classFilter = static_cast< Relevance >( m_classFilterCombo.GetCurSel() );
	std::vector< CThemeClass* > classes; classes.reserve( m_themeStore.m_classes.size() );

	for ( std::vector< CThemeClass* >::const_iterator itClass = m_themeStore.m_classes.begin(); itClass != m_themeStore.m_classes.end(); ++itClass )
		if ( ( *itClass )->GetRelevance() <= classFilter )
			classes.push_back( *itClass );

	std::sort( classes.begin(), classes.begin(), pred::LessBy< pred::CompareRelevance >() );

	m_classCombo.ResetContent();

	for ( std::vector< CThemeClass* >::const_iterator itClass = classes.begin(); itClass != classes.end(); ++itClass )
		if ( ( *itClass )->GetRelevance() <= classFilter )
			m_classCombo.AddString( ( *itClass )->m_className.c_str() );
			
	if ( !ui::SetComboEditText( m_classCombo, selClass.c_str() ).first )
		m_classCombo.SetCurSel( 0 );

	static const std::tstring labelFmt = str::Load( IDC_CLASS_LABEL );
	ui::SetDlgItemText( this, IDC_CLASS_LABEL,
		str::Format( labelFmt.c_str(), hlp::FormatCounts( classes.size(), m_themeStore.m_classes.size() ).c_str() ) );
}

void CMainDialog::SetupPartsAndStatesTree( void )
{
	std::tstring selText;
	if ( HTREEITEM hSelItem = m_partStateTree.GetSelectedItem() )
		selText = m_partStateTree.GetItemText( hSelItem );

	const CThemeClass* pThemeClass = m_themeStore.FindClass( ui::GetComboSelText( m_classCombo ).c_str() );
	ASSERT_PTR( pThemeClass );
	Relevance partsStatesFilter = static_cast< Relevance >( m_partsFilterCombo.GetCurSel() );
	unsigned int partCount = 0, stateCount = 0, totalStateCount = 0;

	++m_internalChange;

	m_partStateTree.DeleteAllItems();

	for ( std::vector< CThemePart* >::const_iterator itPart = pThemeClass->m_parts.begin();
		  itPart != pThemeClass->m_parts.end(); ++itPart )
		if ( ( *itPart )->GetRelevance() <= partsStatesFilter )
		{
			HTREEITEM hPartItem = m_partStateTree.InsertItem(
				TVIF_TEXT | TVIF_PARAM | TVIF_STATE, ( *itPart )->m_partName.c_str(), 0, 0,
				TVIS_BOLD | TVIS_EXPANDED, TVIS_BOLD | TVIS_EXPANDED, (LPARAM)*itPart,
				TVI_ROOT, TVI_LAST );
			++partCount;

			for ( std::vector< CThemeState >::const_iterator itState = ( *itPart )->m_states.begin();
				  itState != ( *itPart )->m_states.end(); ++itState, ++totalStateCount )
				if ( itState->GetRelevance() <= partsStatesFilter )
				{
					m_partStateTree.InsertItem(
						TVIF_TEXT | TVIF_PARAM | TVIF_STATE, itState->m_stateName.c_str(), 0, 0, 0, 0, (LPARAM)&*itState,
						hPartItem, TVI_LAST );
					++stateCount;
				}
		}

	ui::SortCompareTreeChildren( pred::CompareRelevance(), m_partStateTree, TVI_ROOT, Deep );

	--m_internalChange;
	m_partStateTree.SelectItem( m_partStateTree.GetChildItem( TVI_ROOT ) );

	HTREEITEM hSelItem = ui::FindTreeItem( m_partStateTree, selText );
	if ( NULL == hSelItem )
		hSelItem = m_partStateTree.GetChildItem( TVI_ROOT );
	m_partStateTree.SelectItem( hSelItem );

	static const std::tstring labelFmt = str::Load( IDC_PARTS_AND_STATES_LABEL );
	ui::SetDlgItemText( this, IDC_PARTS_AND_STATES_LABEL,
		str::Format( labelFmt.c_str(),
			hlp::FormatCounts( partCount, pThemeClass->m_parts.size() ).c_str(),
			hlp::FormatCounts( stateCount, totalStateCount ).c_str() ) );
}

void CMainDialog::OutputCurrentTheme( void )
{
	CThemeContext selTheme = GetSelThemeContext();

	ui::SetDlgItemText( this, IDC_PART_EDIT, selTheme.FormatPart() );
	ui::SetDlgItemText( this, IDC_PART_NUMBER_EDIT, selTheme.FormatPart( true ) );
	ui::SetDlgItemText( this, IDC_STATE_EDIT, selTheme.FormatState() );
	ui::SetDlgItemText( this, IDC_STATE_NUMBER_EDIT, selTheme.FormatState( true ) );

	CThemeItem themeItem = selTheme.GetThemeItem();

	for ( int i = 0; i != SampleCount; ++i )
		m_samples[ i ].SetThemeItem( themeItem );
}

CThemeContext CMainDialog::GetSelThemeContext( void ) const
{
	CThemeContext selTheme;

	selTheme.m_pClass = m_themeStore.FindClass( ui::GetComboSelText( m_classCombo ).c_str() );

	if ( HTREEITEM hSelItem = m_partStateTree.GetSelectedItem() )
		if ( IThemeNode* pNode = m_partStateTree.GetItemObject< IThemeNode >( hSelItem ) )
			switch ( pNode->GetThemeNode() )
			{
				case IThemeNode::State:
					selTheme.m_pState = static_cast< CThemeState* >( pNode );
					selTheme.m_pPart = static_cast< CThemePart* >( (IThemeNode*)m_partStateTree.GetItemData( m_partStateTree.GetParentItem( hSelItem ) ) );
					break;
				case IThemeNode::Part:
					selTheme.m_pPart = static_cast< CThemePart* >( pNode );
					break;
			}

	return selTheme;
}

void CMainDialog::DoDataExchange( CDataExchange* pDX )
{
	bool firstInit = NULL == m_classCombo.m_hWnd;

	DDX_Control( pDX, IDC_CLASS_COMBO, m_classCombo );
	DDX_Control( pDX, IDC_PARTS_AND_STATES_TREE, m_partStateTree );
	DDX_Control( pDX, IDC_CLASS_FILTER_COMBO, m_classFilterCombo );
	DDX_Control( pDX, IDC_PARTS_FILTER_COMBO, m_partsFilterCombo );
	DDX_Control( pDX, IDC_TINY_SAMPLE_STATIC, m_samples[ Tiny ] );
	DDX_Control( pDX, IDC_SMALL_SAMPLE_STATIC, m_samples[ Small ] );
	DDX_Control( pDX, IDC_MEDIUM_SAMPLE_STATIC, m_samples[ Medium ] );
	DDX_Control( pDX, IDC_LARGE_SAMPLE_STATIC, m_samples[ Large ] );
	DDX_Control( pDX, IDC_BK_COLOR_COMBO, m_bkColorCombo );
	m_toolbar.DDX_Placeholder( pDX, IDC_STRIP_BAR_1, H_AlignLeft | V_AlignTop );

	if ( !pDX->m_bSaveAndValidate )
	{
		if ( firstInit )
		{
			m_classFilterCombo.SetCurSel( AfxGetApp()->GetProfileInt( reg::section, reg::entry_classFilter, ObscureRelevance ) );
			m_partsFilterCombo.SetCurSel( AfxGetApp()->GetProfileInt( reg::section, reg::entry_partsFilter, ObscureRelevance ) );
			ui::LoadHistoryCombo( m_bkColorCombo, reg::section, reg::entry_bkColorHistory, NULL );
			ui::SetDlgItemText( this, IDC_BK_COLOR_COMBO, m_options.m_bkColorText );

			m_options.UpdateControls( this );			// update check-box buttons

			SetupClassesCombo();
			SetupPartsAndStatesTree();

			std::tstring selText = AfxGetApp()->GetProfileString( reg::section, reg::entry_selTreeItem, NULL );
			if ( HTREEITEM hSelItem = ui::FindTreeItem( m_partStateTree, selText ) )
			{
				m_partStateTree.SelectItem( hSelItem );
				OutputCurrentTheme();
			}
			GotoDlgCtrl( &m_partStateTree );
		}
	}

	CBaseMainDialog::DoDataExchange( pDX );
}


// message handlers

BEGIN_MESSAGE_MAP( CMainDialog, CBaseMainDialog )
	ON_WM_DESTROY()
	ON_CBN_SELCHANGE( IDC_CLASS_COMBO, OnCbnSelChange_ClassCombo )
	ON_NOTIFY( TVN_SELCHANGED, IDC_PARTS_AND_STATES_TREE, OnTVnSelChanged_PartStateTree )
	ON_CBN_SELCHANGE( IDC_CLASS_FILTER_COMBO, OnCbnSelChange_ClassFilterCombo )
	ON_CBN_SELCHANGE( IDC_PARTS_FILTER_COMBO, OnCbnSelChange_PartsFilterCombo )
	ON_COMMAND( ID_EDIT_COPY, OnEditCopy )
	ON_UPDATE_COMMAND_UI( ID_EDIT_COPY, OnUpdateEditCopy )
	ON_COMMAND_RANGE( ID_COPY_THEME, ID_COPY_THEME_PART_AND_STATE, OnCopyTheme )
	ON_UPDATE_COMMAND_UI_RANGE( ID_COPY_THEME, ID_COPY_THEME_PART_AND_STATE, OnUpdateCopyTheme )
END_MESSAGE_MAP()

BOOL CMainDialog::OnCmdMsg( UINT id, int code, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo )
{
	return
		m_options.OnCmdMsg( id, code, pExtra, pHandlerInfo ) ||
		CBaseMainDialog::OnCmdMsg( id, code, pExtra, pHandlerInfo );
}

void CMainDialog::OnDestroy( void )
{
	std::tstring selText;
	if ( HTREEITEM hSelItem = m_partStateTree.GetSelectedItem() )
		selText = m_partStateTree.GetItemText( hSelItem );

	AfxGetApp()->WriteProfileString( reg::section, reg::entry_selClass, ui::GetComboSelText( m_classCombo ).c_str() );
	AfxGetApp()->WriteProfileString( reg::section, reg::entry_selTreeItem, selText.c_str() );
	AfxGetApp()->WriteProfileInt( reg::section, reg::entry_classFilter, m_classFilterCombo.GetCurSel() );
	AfxGetApp()->WriteProfileInt( reg::section, reg::entry_partsFilter, m_partsFilterCombo.GetCurSel() );
	ui::SaveHistoryCombo( m_bkColorCombo, reg::section, reg::entry_bkColorHistory );

	CBaseMainDialog::OnDestroy();
}

void CMainDialog::OnCbnSelChange_ClassCombo( void )
{
	SetupPartsAndStatesTree();
}

void CMainDialog::OnTVnSelChanged_PartStateTree( NMHDR* pNmHdr, LRESULT* pResult )
{
	UNUSED_ALWAYS( pNmHdr );

	if ( 0 == m_internalChange )
		OutputCurrentTheme();

	*pResult = 0;
}

void CMainDialog::OnCbnSelChange_ClassFilterCombo( void )
{
	SetupClassesCombo();
	SetupPartsAndStatesTree();
	OutputCurrentTheme();
}

void CMainDialog::OnCbnSelChange_PartsFilterCombo( void )
{
	SetupPartsAndStatesTree();
	OutputCurrentTheme();
}

void CMainDialog::OnEditCopy( void )
{
	m_partStateTree.Copy();
}

void CMainDialog::OnUpdateEditCopy( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( m_partStateTree.GetSelectedItem() != NULL );
}

void CMainDialog::OnCopyTheme( UINT cmdId )
{
	CThemeContext selTheme = GetSelThemeContext();
	std::tstring text = ID_COPY_THEME == cmdId ? selTheme.FormatTheme() : selTheme.FormatThemePartAndState();

	CClipboard::CopyText( text, this );
}

void CMainDialog::OnUpdateCopyTheme( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( GetSelThemeContext().IsValid() );
}
