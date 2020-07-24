
#include "stdafx.h"
#include "MainDialog.h"
#include "BrowseThemesDialog.h"
#include "Application.h"
#include "ThemeContext.h"
#include "ThemeCustomDraw.h"
#include "ThemeStore.h"
#include "utl/StringUtilities.h"
#include "utl/SubjectPredicates.h"
#include "utl/UI/Clipboard.h"
#include "utl/UI/MenuUtilities.h"
#include "utl/UI/UtilitiesEx.h"
#include "utl/UI/StdColors.h"
#include "utl/UI/resource.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#include "utl/UI/ReportListControl.hxx"


namespace reg
{
	static const TCHAR section_settings[] = _T("Settings");
	static const TCHAR entry_selClass[] = _T("SelClass");
	static const TCHAR entry_selTreeItem[] = _T("SelTreeItem");
	static const TCHAR entry_bkColorHistory[] = _T("BkColorHistory");
	static const TCHAR entry_classFilter[] = _T("ClassFilter");
	static const TCHAR entry_partsFilter[] = _T("PartsFilter");
}


namespace layout
{
	enum { TopPct = 30, BottomPct = 100 - TopPct, SizeSamplesPct = 25, MoveSamplesPct = SizeSamplesPct * 15 / 10 /* x1.5 */ };

	static CLayoutStyle styles[] =
	{
		{ IDC_THEME_CLASS_LIST, pctSizeY( TopPct ) },

		{ IDC_PARTS_AND_STATES_LABEL, pctMoveY( TopPct ) },
		{ IDC_PARTS_FILTER_STATIC, pctMoveY( TopPct ) },
		{ IDC_PARTS_FILTER_COMBO, pctMoveY( TopPct ) },
		{ IDC_PARTS_AND_STATES_TREE, pctMoveY( TopPct ) | pctSizeY( BottomPct ) },

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
		{ IDC_BROWSE_THEMES_BUTTON, MoveY },
		{ IDCANCEL, Move }
	};
}


CMainDialog::CMainDialog( COptions* pOptions, const CThemeStore* pThemeStore )
	: CBaseMainDialog( IDD_MAIN_DIALOG )
	, m_pOptions( pOptions )
	, m_pThemeStore( pThemeStore )
	, m_pSelClass( NULL )
	, m_pSelNode( NULL )
	, m_classList( IDC_THEME_CLASS_LIST )
{
	m_regSection = _T("MainDialog");
	RegisterCtrlLayout( ARRAY_PAIR( layout::styles ) );

	m_pOptions->SetCallback( this );

	m_classList.SetSection( _T("MainDialog\\ClassList") );
	m_classList.SetTextEffectCallback( this );
	m_classList.AddRecordCompare( pred::NewComparator( pred::CompareCode() ) );							// default row item comparator
	m_classList.AddColumnCompare( RelevanceTag, pred::NewComparator( pred::CompareRelevance() ) );		// order date-time descending by default

	m_partStateTree.SetTextEffectCallback( this );
	m_partStateTree.SetTrackMenuTarget( this );
	m_partStateTree.GetCtrlAccel().Load( IDC_PARTS_AND_STATES_TREE );
	ui::LoadPopupMenu( m_partStateTree.GetContextMenu(), IDR_CONTEXT_MENU, app::ListTreePopup );

	m_toolbar.GetStrip()
		.AddButton( ID_COPY_THEME )
		.AddButton( ID_COPY_THEME_PART_AND_STATE )
		.AddSeparator()
		.AddButton( ID_PREVIEW_THEME_GLYPHS_CHECK )
		.AddButton( ID_USE_EXPLORER_THEME_CHECK );

	for ( int i = 0; i != SampleCount; ++i )
		m_samples[ i ].SetOptions( m_pOptions );

	m_samples[ Tiny ].m_coreSize = CSize( 10, 10 );
	m_samples[ Medium ].SetSizeToContentMode( IDC_CORE_SIZE_STATIC );
	m_samples[ Large ].m_sampleText = _T("Some Sample Themed Text");

	m_pListCustomDraw.reset( CThemeCustomDraw::MakeListCustomDraw( m_pOptions ) );
	m_pTreeCustomDraw.reset( CThemeCustomDraw::MakeTreeCustomDraw( m_pOptions ) );

	UpdateGlyphPreview();
	UpdateExplorerTheme();
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

	m_classList.Invalidate();
	m_partStateTree.Invalidate();
}

void CMainDialog::UpdateGlyphPreview( void )
{
	m_classList.SetCustomImageDraw( m_pOptions->m_previewThemeGlyphs ? m_pListCustomDraw.get() : NULL );
	m_partStateTree.SetCustomImageDraw( m_pOptions->m_previewThemeGlyphs ? m_pTreeCustomDraw.get() : NULL );
}

void CMainDialog::UpdateExplorerTheme( void )
{
	m_classList.SetUseExplorerTheme( m_pOptions->m_useExplorerTheme );
	m_partStateTree.SetUseExplorerTheme( m_pOptions->m_useExplorerTheme );
}

void CMainDialog::CombineTextEffectAt( ui::CTextEffect& rTextEffect, LPARAM rowKey, int subItem, CListLikeCtrlBase* pCtrl ) const
{
	const IThemeNode* pThemeNode = NULL;

	if ( &m_classList == pCtrl )
	{
		pThemeNode = m_classList.AsPtr< IThemeNode >( rowKey );

		if ( 0 == subItem )
			rTextEffect.m_fontEffect = ui::Bold;
	}
	if ( &m_partStateTree == pCtrl )
		pThemeNode = m_partStateTree.GetItemObject< IThemeNode >( reinterpret_cast< HTREEITEM >( rowKey ) );

	if ( 0 == subItem )
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

void CMainDialog::SetupClassesList( void )
{
	{
		CScopedLockRedraw freeze( &m_classList );
		CScopedInternalChange internalChange( &m_classList );

		m_classList.DeleteAllItems();

		Relevance classFilter = static_cast< Relevance >( m_classFilterCombo.GetCurSel() );

		unsigned int index = 0;
		for ( CThemeClass* pClass : m_pThemeStore->m_classes )
			if ( pClass->GetRelevance() <= classFilter )
			{
				m_classList.InsertObjectItem( index, pClass );		// PathName
				m_classList.SetSubItemText( index, RelevanceTag, GetTags_Relevance().FormatUi( pClass->GetRelevance() ) );
				++index;
			}

		m_classList.InitialSortList();		// store original order and sort by current criteria
	}

	if ( NULL == m_pSelClass || !m_classList.Select( m_pSelClass ) )
		m_classList.Select( m_pSelClass = m_classList.GetObjectAt< CThemeClass >( 0 ) );

	ui::SetDlgItemText( this, IDC_CLASS_LABEL,
		str::Format( _T("Theme &Class (%s):"), hlp::FormatCounts( m_classList.GetItemCount(), m_pThemeStore->m_classes.size() ).c_str() ) );
}

void CMainDialog::SetupPartsAndStatesTree( void )
{
	unsigned int partCount = 0, stateCount = 0, totalStateCount = 0;
	ASSERT_PTR( m_pSelClass );

	{
		CScopedLockRedraw freeze( &m_partStateTree );
		CScopedInternalChange internalChange( &m_partStateTree );

		Relevance partsStatesFilter = static_cast< Relevance >( m_partsFilterCombo.GetCurSel() );

		m_partStateTree.DeleteAllItems();

		for ( CThemePart* pPart : m_pSelClass->m_parts )
			if ( pPart->GetRelevance() <= partsStatesFilter )
			{
				HTREEITEM hPartItem = m_partStateTree.InsertObjectItem( TVI_ROOT, pPart, ui::Transparent_Image, TVIS_BOLD | TVIS_EXPANDED );
				++partCount;

				for ( CThemeState* pState : pPart->m_states )
				{
					if ( pState->GetRelevance() <= partsStatesFilter )
					{
						m_partStateTree.InsertObjectItem( hPartItem, pState );
						++stateCount;
					}

					++totalStateCount;
				}
			}

		ui::SortCompareTreeChildren( pred::CompareRelevance(), m_partStateTree, TVI_ROOT, Deep );
	}

	if ( NULL == m_pSelNode || !m_partStateTree.SetSelected( m_pSelNode ) )
		m_partStateTree.SetSelected( m_pSelNode = m_partStateTree.GetItemObject< IThemeNode >( m_partStateTree.GetChildItem( TVI_ROOT ) ) );

	ui::SetDlgItemText( this, IDC_PARTS_AND_STATES_LABEL,
		str::Format( _T("&Parts (%s) && States (%s):"),
			hlp::FormatCounts( partCount, m_pSelClass->m_parts.size() ).c_str(),
			hlp::FormatCounts( stateCount, totalStateCount ).c_str() ) );

	OutputCurrentTheme();
}

void CMainDialog::OutputCurrentTheme( void )
{
	CThemeContext selTheme = GetSelThemeContext();

	ui::SetDlgItemText( this, IDC_CLASS_EDIT, m_classList.FormatCode( m_pSelClass ) );
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
	selTheme.m_pClass = m_pSelClass;

	if ( m_pSelNode != NULL )
		switch ( m_pSelNode->GetNodeType() )
		{
			case IThemeNode::State:
				selTheme.m_pState = static_cast< CThemeState* >( m_pSelNode );
				selTheme.m_pPart = selTheme.m_pState->GetParentAs< CThemePart >();
				break;
			case IThemeNode::Part:
				selTheme.m_pPart = static_cast< CThemePart* >( m_pSelNode );
				break;
		}

	return selTheme;
}

void CMainDialog::LoadSelItems( void )
{
	std::tstring selText = AfxGetApp()->GetProfileString( reg::section_settings, reg::entry_selClass ).GetString();
	if ( !selText.empty() )
	{
		m_pSelClass = m_pThemeStore->FindClass( selText );
		if ( m_pSelClass != NULL )
		{
			selText = AfxGetApp()->GetProfileString( reg::section_settings, reg::entry_selTreeItem ).GetString();
			if ( !selText.empty() )
				m_pSelNode = m_pSelClass->FindNode( selText );
		}
	}
}

void CMainDialog::DoDataExchange( CDataExchange* pDX )
{
	bool firstInit = NULL == m_classList.m_hWnd;

	DDX_Control( pDX, IDC_THEME_CLASS_LIST, m_classList );
	DDX_Control( pDX, IDC_PARTS_AND_STATES_TREE, m_partStateTree );
	DDX_Control( pDX, IDC_CLASS_FILTER_COMBO, m_classFilterCombo );
	DDX_Control( pDX, IDC_PARTS_FILTER_COMBO, m_partsFilterCombo );
	DDX_Control( pDX, IDC_TINY_SAMPLE_STATIC, m_samples[ Tiny ] );
	DDX_Control( pDX, IDC_SMALL_SAMPLE_STATIC, m_samples[ Small ] );
	DDX_Control( pDX, IDC_MEDIUM_SAMPLE_STATIC, m_samples[ Medium ] );
	DDX_Control( pDX, IDC_LARGE_SAMPLE_STATIC, m_samples[ Large ] );
	DDX_Control( pDX, IDC_BK_COLOR_COMBO, m_bkColorCombo );
	m_toolbar.DDX_Placeholder( pDX, IDC_STRIP_BAR_1, H_AlignLeft | V_AlignTop );

	if ( firstInit )
	{
		ui::WriteComboItems( m_classFilterCombo, GetTags_Relevance().GetUiTags() );
		ui::WriteComboItems( m_partsFilterCombo, GetTags_Relevance().GetUiTags() );

		m_classFilterCombo.SetCurSel( AfxGetApp()->GetProfileInt( reg::section_settings, reg::entry_classFilter, ObscureRelevance ) );
		m_partsFilterCombo.SetCurSel( AfxGetApp()->GetProfileInt( reg::section_settings, reg::entry_partsFilter, ObscureRelevance ) );
		ui::LoadHistoryCombo( m_bkColorCombo, reg::section_settings, reg::entry_bkColorHistory, NULL );
		ui::SetDlgItemText( this, IDC_BK_COLOR_COMBO, m_pOptions->m_bkColorText );

		m_pOptions->UpdateControls( this );			// update check-box buttons

		LoadSelItems();
		SetupClassesList();
		SetupPartsAndStatesTree();

		std::tstring selText = AfxGetApp()->GetProfileString( reg::section_settings, reg::entry_selTreeItem, NULL );
		if ( HTREEITEM hSelItem = ui::FindTreeItem( m_partStateTree, selText ) )
		{
			m_partStateTree.SelectItem( hSelItem );
			OutputCurrentTheme();
		}
		GotoDlgCtrl( &m_partStateTree );
	}

	__super::DoDataExchange( pDX );
}


// message handlers

BEGIN_MESSAGE_MAP( CMainDialog, CBaseMainDialog )
	ON_WM_DESTROY()
	ON_NOTIFY( LVN_ITEMCHANGED, IDC_THEME_CLASS_LIST, OnLvnItemChanged_ThemeClass )
	ON_NOTIFY( TVN_SELCHANGED, IDC_PARTS_AND_STATES_TREE, OnTvnSelChanged_PartStateTree )
	ON_CBN_SELCHANGE( IDC_CLASS_FILTER_COMBO, OnCbnSelChange_ClassFilterCombo )
	ON_CBN_SELCHANGE( IDC_PARTS_FILTER_COMBO, OnCbnSelChange_PartsFilterCombo )
	ON_COMMAND( ID_EDIT_COPY, OnEditCopy )
	ON_UPDATE_COMMAND_UI( ID_EDIT_COPY, OnUpdateEditCopy )
	ON_COMMAND_RANGE( ID_COPY_THEME, ID_COPY_THEME_PART_AND_STATE, OnCopyTheme )
	ON_UPDATE_COMMAND_UI_RANGE( ID_COPY_THEME, ID_COPY_THEME_PART_AND_STATE, OnUpdateCopyTheme )
	ON_BN_CLICKED( IDC_BROWSE_THEMES_BUTTON, OnBrowseThemes )
END_MESSAGE_MAP()

BOOL CMainDialog::OnCmdMsg( UINT id, int code, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo )
{
	return
		m_pOptions->OnCmdMsg( id, code, pExtra, pHandlerInfo ) ||
		CBaseMainDialog::OnCmdMsg( id, code, pExtra, pHandlerInfo );
}

void CMainDialog::OnDestroy( void )
{
	AfxGetApp()->WriteProfileString( reg::section_settings, reg::entry_selClass, m_classList.FormatCode( m_pSelClass ).c_str() );
	AfxGetApp()->WriteProfileString( reg::section_settings, reg::entry_selTreeItem, m_partStateTree.FormatCode( m_pSelNode ).c_str() );
	AfxGetApp()->WriteProfileInt( reg::section_settings, reg::entry_classFilter, m_classFilterCombo.GetCurSel() );
	AfxGetApp()->WriteProfileInt( reg::section_settings, reg::entry_partsFilter, m_partsFilterCombo.GetCurSel() );
	ui::SaveHistoryCombo( m_bkColorCombo, reg::section_settings, reg::entry_bkColorHistory );

	CBaseMainDialog::OnDestroy();
}

void CMainDialog::OnLvnItemChanged_ThemeClass( NMHDR* pNmHdr, LRESULT* pResult )
{
	NMLISTVIEW* pNmList = (NMLISTVIEW*)pNmHdr;
	*pResult = 0;

	if ( !m_classList.IsInternalChange() )
		if ( CReportListControl::IsSelectionChangeNotify( pNmList ) && HasFlag( pNmList->uNewState, LVIS_SELECTED ) )		// new item selected?
		{
			m_pSelClass = m_classList.GetSelected< CThemeClass >();
			SetupPartsAndStatesTree();
		}
}

void CMainDialog::OnTvnSelChanged_PartStateTree( NMHDR* pNmHdr, LRESULT* pResult )
{
	UNUSED_ALWAYS( pNmHdr );
	*pResult = 0;

	if ( !m_partStateTree.IsInternalChange() )
	{
		m_pSelNode = m_partStateTree.GetSelected< IThemeNode >();
		OutputCurrentTheme();
	}
}

void CMainDialog::OnCbnSelChange_ClassFilterCombo( void )
{
	SetupClassesList();
	SetupPartsAndStatesTree();
}

void CMainDialog::OnCbnSelChange_PartsFilterCombo( void )
{
	SetupPartsAndStatesTree();
}

void CMainDialog::OnEditCopy( void )
{
	m_partStateTree.Copy();
}

void CMainDialog::OnUpdateEditCopy( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( m_pSelNode != NULL );
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

void CMainDialog::OnBrowseThemes( void )
{
	Relevance relevanceFilter = static_cast< Relevance >( std::max( m_classFilterCombo.GetCurSel(), m_partsFilterCombo.GetCurSel() ) );
	CBrowseThemesDialog dlg( m_pOptions, m_pThemeStore, relevanceFilter, this );

	dlg.SetSelectedNode( m_pSelNode );
	if ( IDOK == dlg.DoModal() )
		if ( IThemeNode* pSelNode = dlg.GetSelectedNode() )
		{
			CThemeClass* pSelClass = dlg.GetSelectedClass();
			m_classList.Select( pSelClass );

			if ( dlg.GetSelectedNode() == pSelClass )
				GotoDlgCtrl( &m_classList );
			else
				if ( m_partStateTree.SetSelected( m_pSelNode = pSelNode ) )
					GotoDlgCtrl( &m_partStateTree );
				else
					ui::BeepSignal( MB_ICONWARNING );		// no matching item due to relevance filter
		}
}
