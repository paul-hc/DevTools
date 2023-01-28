
#include "pch.h"
#include "BrowseThemesDialog.h"
#include "Options.h"
#include "ThemeCustomDraw.h"
#include "ThemeStore.h"
#include "utl/SubjectPredicates.h"
#include "utl/UI/WndUtilsEx.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace hlp
{
	std::tstring FormatCounts( size_t count, size_t total )
	{
		return str::Format( count != total ? _T("%d/%d") : _T("%d"), count, total );
	}
}


namespace layout
{
	static CLayoutStyle s_styles[] =
	{
		{ IDC_PARTS_FILTER_STATIC, MoveX },
		{ IDC_PARTS_FILTER_COMBO, MoveX },
		{ IDC_PARTS_AND_STATES_TREE, Size },
		{ IDOK, Move },
		{ IDCANCEL, Move }
	};
}


CBrowseThemesDialog::CBrowseThemesDialog( const COptions* pOptions, const CThemeStore* pThemeStore, Relevance relevanceFilter, CWnd* pParent )
	: CBaseMainDialog( IDD_BROWSE_THEMES_DIALOG, pParent )
	, m_pOptions( pOptions )
	, m_pThemeStore( pThemeStore )
	, m_relevanceFilter( relevanceFilter )
	, m_pTreeCustomDraw( CThemeCustomDraw::MakeTreeCustomDraw( m_pOptions ) )
	, m_pSelNode( NULL )
{
	m_regSection = _T("BrowseThemesDialog");
	RegisterCtrlLayout( ARRAY_PAIR( layout::s_styles ) );

	m_themesTree.SetTextEffectCallback( this );
	m_themesTree.SetCustomImageDraw( m_pOptions->m_previewThemeGlyphs ? m_pTreeCustomDraw.get() : NULL );
	m_themesTree.SetUseExplorerTheme( m_pOptions->m_useExplorerTheme );
}

CBrowseThemesDialog::~CBrowseThemesDialog()
{
}

CThemeClass* CBrowseThemesDialog::GetSelectedClass( void ) const
{
	if ( NULL == m_pSelNode )
		return NULL;

	if ( CThemeClass* pSelClass = dynamic_cast< CThemeClass* >( m_pSelNode ) )
		return pSelClass;

	if ( CThemePart* pSelPart = dynamic_cast< CThemePart* >( m_pSelNode ) )
		return pSelPart->GetParentAs< CThemeClass >();

	CThemeState* pSelState = checked_static_cast< CThemeState* >( m_pSelNode );

	return pSelState->GetParentAs< CThemePart >()->GetParentAs< CThemeClass >();
}

void CBrowseThemesDialog::CombineTextEffectAt( ui::CTextEffect& rTextEffect, LPARAM rowKey, int subItem, CListLikeCtrlBase* pCtrl ) const
{
	subItem, pCtrl;
	const IThemeNode* pThemeNode = m_themesTree.GetItemObject< IThemeNode >( reinterpret_cast< HTREEITEM >( rowKey ) );

	if ( is_a< CThemeClass >( pThemeNode ) )
		rTextEffect.m_fontEffect = ui::Bold | ui::Underline;
	else if ( is_a< CThemePart >( pThemeNode ) )
		rTextEffect.m_fontEffect = ui::Bold;

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

void CBrowseThemesDialog::SetupTree( void )
{
	{
		CScopedLockRedraw freeze( &m_themesTree );
		CScopedInternalChange internalChange( &m_themesTree );

		m_themesTree.DeleteAllItems();

		Relevance relevanceFilter = static_cast< Relevance >( m_filterCombo.GetCurSel() );

		for ( CThemeClass* pClass : m_pThemeStore->m_classes )
			if ( pClass->GetRelevance() <= relevanceFilter )
			{
				HTREEITEM hClassItem = m_themesTree.InsertObjectItem( TVI_ROOT, pClass, ui::Transparent_Image, TVIS_BOLD | TVIS_EXPANDED );

				for ( CThemePart* pPart : pClass->m_parts )
					if ( pPart->GetRelevance() <= relevanceFilter )
					{
						HTREEITEM hPartItem = m_themesTree.InsertObjectItem( hClassItem, pPart, ui::Transparent_Image, TVIS_BOLD | TVIS_EXPANDED );

						for ( CThemeState* pState : pPart->m_states )
							if ( pState->GetRelevance() <= relevanceFilter )
								m_themesTree.InsertObjectItem( hPartItem, pState );
					}
			}

		ui::SortCompareTreeChildren( pred::TCompareRelevance(), m_themesTree, TVI_ROOT, Deep );
	}

	if ( NULL == m_pSelNode || !m_themesTree.SetSelected( m_pSelNode ) )
	{
		HTREEITEM hFirstItem = m_themesTree.GetChildItem( TVI_ROOT );
		m_pSelNode = m_themesTree.GetItemObject< IThemeNode >( hFirstItem );
		m_themesTree.SelectItem( hFirstItem );
	}

	HTREEITEM hSelItem = NULL;
	if ( m_pSelNode != NULL )
		hSelItem = m_themesTree.FindItemWithObject( m_pSelNode );
	if ( NULL == hSelItem )
		hSelItem = m_themesTree.GetChildItem( TVI_ROOT );

	m_themesTree.SelectItem( hSelItem );

	ui::SetDlgItemText( this, IDC_PARTS_AND_STATES_LABEL,
		str::Format( _T("&Theme items (%s):"), hlp::FormatCounts( m_themesTree.GetCount(), m_pThemeStore->GetTotalCount() ).c_str() ) );
}

void CBrowseThemesDialog::DoDataExchange( CDataExchange* pDX )
{
	bool firstInit = NULL == m_filterCombo.m_hWnd;

	DDX_Control( pDX, IDC_PARTS_FILTER_COMBO, m_filterCombo );
	DDX_Control( pDX, IDC_PARTS_AND_STATES_TREE, m_themesTree );

	if ( firstInit )
	{
		ui::WriteComboItems( m_filterCombo, GetTags_Relevance().GetUiTags() );
		m_filterCombo.SetCurSel( m_relevanceFilter );
		SetupTree();
	}

	__super::DoDataExchange( pDX );
}


// message handlers

BEGIN_MESSAGE_MAP( CBrowseThemesDialog, CBaseMainDialog )
	ON_CBN_SELCHANGE( IDC_PARTS_FILTER_COMBO, OnCbnSelChange_FilterCombo )
	ON_NOTIFY( TVN_SELCHANGED, IDC_PARTS_AND_STATES_TREE, OnTvnSelChanged_ThemesTree )
END_MESSAGE_MAP()

void CBrowseThemesDialog::OnCbnSelChange_FilterCombo( void )
{
	SetupTree();
}

void CBrowseThemesDialog::OnTvnSelChanged_ThemesTree( NMHDR* pNmHdr, LRESULT* pResult )
{
	UNUSED_ALWAYS( pNmHdr );
	*pResult = 0;

	if ( !m_themesTree.IsInternalChange() )
		if ( HTREEITEM hSelItem = m_themesTree.GetSelectedItem() )
			SetSelectedNode( m_themesTree.GetItemObject< IThemeNode >( hSelItem ) );
}
