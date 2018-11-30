
#include "stdafx.h"
#include "MainDialog.h"
#include "Application.h"
#include "ThemeContext.h"
#include "ThemeCustomDraw.h"
#include "utl/Clipboard.h"
#include "utl/MenuUtilities.h"
#include "utl/StringUtilities.h"
#include "utl/UtilitiesEx.h"
#include "utl/StdColors.h"
#include "utl/SubjectPredicates.h"
#include "utl/resource.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace reg
{
	static const TCHAR section_dialog[] = _T("MainDialog");
	static const TCHAR section_classList[] = _T("MainDialog\\ClassList");
	static const TCHAR entry_selClass[] = _T("SelClass");
	static const TCHAR entry_selTreeItem[] = _T("SelTreeItem");
	static const TCHAR entry_bkColorHistory[] = _T("BkColorHistory");
	static const TCHAR entry_classFilter[] = _T("ClassFilter");
	static const TCHAR entry_partsFilter[] = _T("PartsFilter");
}


namespace layout
{
	enum { TopPct = 30, BottomPct = 100 - TopPct, SizeSamplesPct = 25, MoveSamplesPct = SizeSamplesPct * 15 / 10 /* x1.5 */ };

	static CLayoutStyle layoutStyles[] =
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
		{ IDCANCEL, Move }
	};
}


CMainDialog::CMainDialog( void )
	: CBaseMainDialog( IDD_MAIN_DIALOG )
	, m_options( this )
	, m_classList( IDC_THEME_CLASS_LIST )
{
	m_regSection = reg::section_dialog;
	RegisterCtrlLayout( layout::layoutStyles, COUNT_OF( layout::layoutStyles ) );
	m_themeStore.SetupNotImplementedThemes();				// mark not implemented themes as NotImplemented

	m_classList.SetSection( reg::section_classList );
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
		m_samples[ i ].SetOptions( &m_options );

	m_samples[ Tiny ].m_coreSize = CSize( 10, 10 );
	m_samples[ Medium ].SetSizeToContentMode( IDC_CORE_SIZE_STATIC );
	m_samples[ Large ].m_sampleText = _T("Some Sample Themed Text");

	InitCustomDraw();
	UpdateGlyphPreview();
	UpdateExplorerTheme();
}

CMainDialog::~CMainDialog()
{
}

void CMainDialog::InitCustomDraw( void )
{
	static const CSize s_themePreviewSize( 40, 24 ), s_themePreviewSizeLarge( 50, 32 );

	m_pListCustomDraw.reset( new CThemeCustomDraw( &m_options ) );
	m_pListCustomDraw->m_imageSize[ ui::SmallGlyph ] = s_themePreviewSize;
	m_pListCustomDraw->m_imageSize[ ui::LargeGlyph ] = s_themePreviewSizeLarge;
	m_pListCustomDraw->m_imageMargin = CSize( 0, 2 );
	m_pListCustomDraw->m_textMargin = 2;

	m_pTreeCustomDraw.reset( new CThemeCustomDraw( &m_options ) );
	m_pTreeCustomDraw->m_imageSize[ ui::SmallGlyph ] = s_themePreviewSize;
	m_pTreeCustomDraw->m_imageMargin = CSize( 0, 1 );
	m_pTreeCustomDraw->m_textMargin = 2;
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
	m_classList.SetCustomImageDraw( m_options.m_previewThemeGlyphs ? m_pListCustomDraw.get() : NULL );
	m_partStateTree.SetCustomImageDraw( m_options.m_previewThemeGlyphs ? m_pTreeCustomDraw.get() : NULL );
}

void CMainDialog::UpdateExplorerTheme( void )
{
	m_classList.SetUseExplorerTheme( m_options.m_useExplorerTheme );
	m_partStateTree.SetUseExplorerTheme( m_options.m_useExplorerTheme );
}

void CMainDialog::CombineTextEffectAt( ui::CTextEffect& rTextEffect, LPARAM rowKey, int subItem, CListLikeCtrlBase* pCtrl ) const
{
	const IThemeNode* pThemeNode = NULL;

	if ( &m_classList == pCtrl )
		pThemeNode = m_classList.AsPtr< IThemeNode >( rowKey );
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
	CScopedListTextSelection sel( &m_classList );
	CScopedLockRedraw freeze( &m_classList );
	CScopedInternalChange internalChange( &m_classList );

	m_classList.DeleteAllItems();

	Relevance classFilter = static_cast< Relevance >( m_classFilterCombo.GetCurSel() );

	unsigned int index = 0;
	for ( auto itClass = m_themeStore.m_classes.begin(); itClass != m_themeStore.m_classes.end(); ++itClass )
		if ( ( *itClass )->GetRelevance() <= classFilter )
		{
			m_classList.InsertObjectItem( index, *itClass );		// PathName
			m_classList.SetSubItemText( index, RelevanceTag, GetTags_Relevance().FormatUi( ( *itClass )->GetRelevance() ) );
			++index;
		}

	m_classList.InitialSortList();		// store original order and sort by current criteria

	if ( sel.m_selTexts.empty() )
	{
		std::tstring selClass = AfxGetApp()->GetProfileString( reg::section_dialog, reg::entry_selClass );
		if ( selClass.empty() )
			selClass = m_classList.GetSubjectAt( 0 )->GetCode();
		sel.m_selTexts.push_back( selClass );
	}

	ui::SetDlgItemText( this, IDC_CLASS_LABEL,
		str::Format( _T("Theme &Class (%s):"), FormatCounts( m_classList.GetItemCount(), m_themeStore.m_classes.size() ).c_str() ) );
}

void CMainDialog::SetupPartsAndStatesTree( void )
{
	CScopedLockRedraw freeze( &m_partStateTree );
	CScopedInternalChange internalChange( &m_partStateTree );

	std::tstring selText;
	if ( HTREEITEM hSelItem = m_partStateTree.GetSelectedItem() )
		selText = m_partStateTree.GetItemText( hSelItem );

	const CThemeClass* pSelThemeClass = m_classList.GetSelected< CThemeClass >();
	ASSERT_PTR( pSelThemeClass );
	Relevance partsStatesFilter = static_cast< Relevance >( m_partsFilterCombo.GetCurSel() );
	unsigned int partCount = 0, stateCount = 0, totalStateCount = 0;

	m_partStateTree.DeleteAllItems();

	for ( auto itPart = pSelThemeClass->m_parts.begin(); itPart != pSelThemeClass->m_parts.end(); ++itPart )
		if ( ( *itPart )->GetRelevance() <= partsStatesFilter )
		{
			HTREEITEM hPartItem = m_partStateTree.InsertObjectItem( TVI_ROOT, *itPart, ui::Transparent_Image, TVIS_BOLD | TVIS_EXPANDED );
			++partCount;

			for ( auto itState = ( *itPart )->m_states.begin(); itState != ( *itPart )->m_states.end(); ++itState, ++totalStateCount )
				if ( ( *itState )->GetRelevance() <= partsStatesFilter )
				{
					m_partStateTree.InsertObjectItem( hPartItem, *itState );
					++stateCount;
				}
		}

	ui::SortCompareTreeChildren( pred::CompareRelevance(), m_partStateTree, TVI_ROOT, Deep );

	m_partStateTree.SelectItem( m_partStateTree.GetChildItem( TVI_ROOT ) );

	HTREEITEM hSelItem = ui::FindTreeItem( m_partStateTree, selText );
	if ( NULL == hSelItem )
		hSelItem = m_partStateTree.GetChildItem( TVI_ROOT );
	m_partStateTree.SelectItem( hSelItem );

	ui::SetDlgItemText( this, IDC_PARTS_AND_STATES_LABEL,
		str::Format( _T("&Parts (%s) && States (%s):"),
			FormatCounts( partCount, pSelThemeClass->m_parts.size() ).c_str(),
			FormatCounts( stateCount, totalStateCount ).c_str() ) );

	if ( hSelItem != NULL )
		OutputCurrentTheme();
}

void CMainDialog::OutputCurrentTheme( void )
{
	CThemeContext selTheme = GetSelThemeContext();

	ui::SetDlgItemText( this, IDC_CLASS_EDIT, m_classList.FormatCode( m_classList.GetSelected< IThemeNode >() ) );
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
	selTheme.m_pClass = m_classList.GetSelected< CThemeClass >();

	if ( HTREEITEM hSelItem = m_partStateTree.GetSelectedItem() )
		if ( IThemeNode* pNode = m_partStateTree.GetItemObject< IThemeNode >( hSelItem ) )
			switch ( pNode->GetNodeType() )
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

std::tstring CMainDialog::FormatCounts( unsigned int count, unsigned int total )
{
	return str::Format( count != total ? _T("%d/%d") : _T("%d"), count, total );
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

	if ( !pDX->m_bSaveAndValidate )
	{
		if ( firstInit )
		{
			m_classFilterCombo.SetCurSel( AfxGetApp()->GetProfileInt( reg::section_dialog, reg::entry_classFilter, ObscureRelevance ) );
			m_partsFilterCombo.SetCurSel( AfxGetApp()->GetProfileInt( reg::section_dialog, reg::entry_partsFilter, ObscureRelevance ) );
			ui::LoadHistoryCombo( m_bkColorCombo, reg::section_dialog, reg::entry_bkColorHistory, NULL );
			ui::SetDlgItemText( this, IDC_BK_COLOR_COMBO, m_options.m_bkColorText );

			m_options.UpdateControls( this );			// update check-box buttons

			SetupClassesList();
			SetupPartsAndStatesTree();

			std::tstring selText = AfxGetApp()->GetProfileString( reg::section_dialog, reg::entry_selTreeItem, NULL );
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
	ON_NOTIFY( LVN_ITEMCHANGED, IDC_THEME_CLASS_LIST, OnLvnItemChanged_ThemeClass )
	ON_NOTIFY( TVN_SELCHANGED, IDC_PARTS_AND_STATES_TREE, OnTvnSelChanged_PartStateTree )
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
	std::tstring selClassText, selItemText;

	selClassText = m_classList.FormatCode( m_classList.GetSelected< utl::ISubject >() );
	if ( HTREEITEM hSelItem = m_partStateTree.GetSelectedItem() )
		selItemText = m_partStateTree.GetItemText( hSelItem );

	AfxGetApp()->WriteProfileString( reg::section_dialog, reg::entry_selClass, selClassText.c_str() );
	AfxGetApp()->WriteProfileString( reg::section_dialog, reg::entry_selTreeItem, selItemText.c_str() );
	AfxGetApp()->WriteProfileInt( reg::section_dialog, reg::entry_classFilter, m_classFilterCombo.GetCurSel() );
	AfxGetApp()->WriteProfileInt( reg::section_dialog, reg::entry_partsFilter, m_partsFilterCombo.GetCurSel() );
	ui::SaveHistoryCombo( m_bkColorCombo, reg::section_dialog, reg::entry_bkColorHistory );

	CBaseMainDialog::OnDestroy();
}

void CMainDialog::OnLvnItemChanged_ThemeClass( NMHDR* pNmHdr, LRESULT* pResult )
{
	NMLISTVIEW* pNmList = (NMLISTVIEW*)pNmHdr;
	*pResult = 0;

	if ( !m_classList.IsInternalChange() )
		if ( CReportListControl::IsSelectionChangeNotify( pNmList ) && HasFlag( pNmList->uNewState, LVIS_SELECTED ) )		// new item selected?
			SetupPartsAndStatesTree();
}

void CMainDialog::OnTvnSelChanged_PartStateTree( NMHDR* pNmHdr, LRESULT* pResult )
{
	UNUSED_ALWAYS( pNmHdr );
	*pResult = 0;

	if ( !m_partStateTree.IsInternalChange() )
		OutputCurrentTheme();
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
