
#include "pch.h"
#include "ToolbarImagesDialog.h"
#include "ImageCommandLookup.h"
#include "WndUtils.h"
#include "resource.h"
#include "utl/ContainerOwnership.h"
#include "utl/StringUtilities.h"
#include "utl/Subject.h"
#include <afxtoolbar.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace reg
{
	static const TCHAR section_dialog[] = _T("utl\\ToolbarImagesDialog");
	static const TCHAR section_MfcPage[] = _T("utl\\ToolbarImagesDialog\\MFC Page");
	static const TCHAR entry_SelItemIndex[] = _T("SelItemIndex");
	static const TCHAR entry_DrawDisabled[] = _T("DrawDisabled");
	static const TCHAR entry_AlphaSrc[] = _T("AlphaSrc");
}

namespace layout
{
	static CLayoutStyle s_styles[] =
	{
		{ IDC_ITEMS_SHEET, Size },
		{ IDOK, Move },
		{ IDCANCEL, Move }
	};
}

CToolbarImagesDialog::CToolbarImagesDialog( CWnd* pParent )
	: CLayoutDialog( IDD_TOOLBAR_IMAGES_DIALOG, pParent )
{
	m_regSection = m_childSheet.m_regSection = reg::section_dialog;
	RegisterCtrlLayout( ARRAY_SPAN( layout::s_styles ) );
	LoadDlgIcon( ID_EDIT_LIST_ITEMS );
	//m_idleUpdateDeep = true;				// for CItemsEditPage::OnSelectedLinesChanged

	m_childSheet.AddPage( new CToolbarImagesPage( CMFCToolBar::GetImages() ) );
}

CToolbarImagesDialog::~CToolbarImagesDialog()
{
}

TRuntimeClassList* CToolbarImagesDialog::GetCustomPages( void )
{
	static TRuntimeClassList s_pagesList;

	if ( s_pagesList.IsEmpty() )
	{
		s_pagesList.AddTail( RUNTIME_CLASS( CToolbarImagesPage ) );
	}

	return &s_pagesList;
}

void CToolbarImagesDialog::DoDataExchange( CDataExchange* pDX )
{
	bool firstInit = nullptr == m_childSheet.m_hWnd;

	if ( firstInit )
	{
	}

	m_childSheet.DDX_DetailSheet( pDX, IDC_ITEMS_SHEET );

	if ( DialogOutput == pDX->m_bSaveAndValidate )
		m_childSheet.OutputPages();

	CLayoutDialog::DoDataExchange( pDX );
}


// message handlers

BEGIN_MESSAGE_MAP( CToolbarImagesDialog, CLayoutDialog )
END_MESSAGE_MAP()


// CToolbarImagesPage class

struct CImageItem : public TBasicSubject
{
	CImageItem( int index = -1, UINT cmdId = 0, const std::tstring* pCmdName = nullptr, const std::tstring* pCmdLiteral = nullptr )
		: m_index( index )
		, m_cmdId( cmdId )
	{
		if ( pCmdName != nullptr )
			m_cmdName = *pCmdName;

		if ( pCmdLiteral != nullptr )
			m_cmdLiteral = *pCmdLiteral;
	}

	virtual const std::tstring& GetCode( void ) const implement { return m_cmdName; }
	virtual std::tstring GetDisplayCode( void ) const { return GetCode(); }
public:
	int m_index;
	UINT m_cmdId;
	std::tstring m_cmdName;
	std::tstring m_cmdLiteral;
};


namespace layout
{
	static CLayoutStyle s_mfcPageStyles[] =
	{
		{ IDC_TOOLBAR_IMAGES_STATIC, SizeX },
		{ IDC_TOOLBAR_IMAGES_LIST, Size },
		{ IDC_GROUP_BOX_1, MoveY },
		{ IDC_ALPHA_SRC_LABEL, MoveY },
		{ IDC_DRAW_DISABLED_CHECK, MoveY },
		{ IDC_ALPHA_SRC_EDIT, MoveY },
		{ IDC_ALPHA_SRC_SPIN, MoveY }
	};
}

IMPLEMENT_DYNCREATE( CToolbarImagesPage, CPropertyPage )

CToolbarImagesPage::CToolbarImagesPage( CMFCToolBarImages* pImages /*= nullptr*/ )
	: CLayoutPropertyPage( IDD_TOOLBAR_IMAGES_PAGE )
	, m_pImages( pImages != nullptr ? pImages : CMFCToolBar::GetImages() )
	, m_imageSize( m_pImages->GetImageSize() )
	, m_selItemIndex( AfxGetApp()->GetProfileInt( reg::section_MfcPage, reg::entry_SelItemIndex, 0 ) )
	, m_drawDisabled( AfxGetApp()->GetProfileInt( reg::section_MfcPage, reg::entry_DrawDisabled, false ) != FALSE )
	, m_alphaSrc( static_cast<BYTE>( AfxGetApp()->GetProfileInt( reg::section_MfcPage, reg::entry_AlphaSrc, 255u ) ) )
	, m_imageListCtrl( IDC_TOOLBAR_IMAGES_LIST )
	, m_imageBoundsSize( CIconSize::GetSizeOf( SmallIcon ) )
{
	RegisterCtrlLayout( ARRAY_SPAN( layout::s_mfcPageStyles ) );
	SetUseLazyUpdateData();			// call UpdateData on page activation change
	ASSERT_PTR( m_pImages );

	m_imageListCtrl.SetSection( reg::section_MfcPage );
	m_imageListCtrl.SetCustomIconDraw( this );
	m_imageListCtrl.SetUseAlternateRowColoring();

	UINT imageCount = m_pImages->GetCount();
	const mfc::CImageCommandLookup* pImageCmds = mfc::CImageCommandLookup::Instance();

	m_imageItems.reserve( imageCount );
	for ( UINT index = 0; index != imageCount; ++index )
	{
		UINT cmdId = pImageCmds->FindCommand( index );
		const std::tstring* pCmdName = pImageCmds->FindCommandName( cmdId );
		const std::tstring* pCmdLiteral = pImageCmds->FindCommandLiteral( cmdId );

		m_imageItems.push_back( new CImageItem( index, cmdId, pCmdName, pCmdLiteral ) );
	}
}

CToolbarImagesPage::~CToolbarImagesPage()
{
	utl::ClearOwningContainer( m_imageItems );
}

void CToolbarImagesPage::OutputList( void )
{
	CScopedInternalChange internalChange( &m_imageListCtrl );

	m_imageListCtrl.DeleteAllItems();

	for ( UINT i = 0; i != m_imageItems.size(); ++i )
	{
		CImageItem* pImageItem = m_imageItems[ i ];

		m_imageListCtrl.InsertObjectItem( i, pImageItem );
		m_imageListCtrl.SetSubItemText( i, Index, num::FormatNumber( pImageItem->m_index ) );
		m_imageListCtrl.SetSubItemText( i, CmdId, pImageItem->m_cmdId != 0 ? str::Format( _T("%d, 0x%04X"), pImageItem->m_cmdId, pImageItem->m_cmdId ) : num::FormatNumber( pImageItem->m_cmdId ) );
		m_imageListCtrl.SetSubItemText( i, CmdLiteral, pImageItem->m_cmdLiteral );
	}

	m_imageListCtrl.InitialSortList();		// store original order and sort by current criteria

	if ( m_selItemIndex != -1 )
	{
		m_imageListCtrl.SetCurSel( m_selItemIndex );
		m_imageListCtrl.EnsureVisible( m_selItemIndex, false );
	}
}

CSize CToolbarImagesPage::GetItemImageSize( ui::GlyphGauge glyphGauge ) const implements(ui::ICustomImageDraw)
{
	switch ( glyphGauge )
	{
		default: ASSERT( false );
		case ui::SmallGlyph:	return CIconSize::GetSizeOf( SmallIcon );
		case ui::LargeGlyph:	return CIconSize::GetSizeOf( LargeIcon );
	}
}

bool CToolbarImagesPage::SetItemImageSize( const CSize& imageBoundsSize ) implements(ui::ICustomImageDraw)
{
	if ( m_imageBoundsSize == imageBoundsSize )
		return false;

	m_imageBoundsSize = imageBoundsSize;
	return true;
}

bool CToolbarImagesPage::DrawItemImage( CDC* pDC, const utl::ISubject* pSubject, const CRect& itemImageRect ) implements(ui::ICustomImageDraw)
{
	CAfxDrawState drawState;

	if ( !m_pImages->PrepareDrawImage( drawState ) )
		return false;

	const CImageItem* pImageItem = checked_static_cast<const CImageItem*>( pSubject );
	CRect imageRect( 0, 0, m_imageSize.cx, m_imageSize.cy );

	ui::CenterRect( imageRect, itemImageRect );

	m_pImages->Draw( pDC, imageRect.left, imageRect.top, pImageItem->m_index, FALSE, m_drawDisabled, FALSE, FALSE, FALSE, m_alphaSrc );
	m_pImages->EndDrawImage( drawState );
	return true;
}

void CToolbarImagesPage::DoDataExchange( CDataExchange* pDX )
{
	bool firstInit = nullptr == m_imageListCtrl.m_hWnd;
	static std::tstring s_imageCountFmt = ui::GetDlgItemText( this, IDC_TOOLBAR_IMAGES_STATIC );

	DDX_Control( pDX, IDC_TOOLBAR_IMAGES_LIST, m_imageListCtrl );
	ui::DDX_Bool( pDX, IDC_DRAW_DISABLED_CHECK, m_drawDisabled );
	ui::DDX_Number( pDX, IDC_ALPHA_SRC_EDIT, m_alphaSrc );

	if ( firstInit )
	{
		CSpinButtonCtrl* pAlphaSrcSpinBtn = ui::GetDlgItemAs<CSpinButtonCtrl>( this, IDC_ALPHA_SRC_SPIN );

		ASSERT_PTR( pAlphaSrcSpinBtn->GetSafeHwnd() );
		pAlphaSrcSpinBtn->SetRange32( 0, 255 );
	}

	if ( DialogOutput == pDX->m_bSaveAndValidate )
	{
		ui::SetDlgItemText( this, IDC_TOOLBAR_IMAGES_STATIC, str::Format( s_imageCountFmt.c_str(), m_pImages->GetCount() ) );
		OutputList();
	}

	CLayoutPropertyPage::DoDataExchange( pDX );
}

BEGIN_MESSAGE_MAP( CToolbarImagesPage, CLayoutPropertyPage )
	ON_WM_DESTROY()
	ON_NOTIFY( LVN_ITEMCHANGED, IDC_TOOLBAR_IMAGES_LIST, OnLvnItemChanged_ImageListCtrl )
	ON_BN_CLICKED( IDC_DRAW_DISABLED_CHECK, OnRedrawImagesList )
	ON_EN_CHANGE( IDC_ALPHA_SRC_EDIT, OnRedrawImagesList )
END_MESSAGE_MAP()

void CToolbarImagesPage::OnDestroy( void )
{
	AfxGetApp()->WriteProfileInt( reg::section_MfcPage, reg::entry_SelItemIndex, m_selItemIndex );
	AfxGetApp()->WriteProfileInt( reg::section_MfcPage, reg::entry_DrawDisabled, m_drawDisabled );
	AfxGetApp()->WriteProfileInt( reg::section_MfcPage, reg::entry_AlphaSrc, m_alphaSrc );

	__super::OnDestroy();
}

void CToolbarImagesPage::OnCopyItems( void )
{
	m_imageListCtrl.Copy( ds::Indexes | ds::ItemsText );
}

void CToolbarImagesPage::OnUpdateCopyItems( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( m_imageListCtrl.AnySelected() );
}

void CToolbarImagesPage::OnLvnItemChanged_ImageListCtrl( NMHDR* pNmHdr, LRESULT* pResult )
{
	NMLISTVIEW* pNmList = (NMLISTVIEW*)pNmHdr;
	*pResult = 0;

	if ( CReportListControl::IsSelectionChangeNotify( pNmList, LVIS_SELECTED | LVIS_FOCUSED ) )
		m_selItemIndex = m_imageListCtrl.GetCurSel();
}

void CToolbarImagesPage::OnRedrawImagesList( void )
{
	if ( nullptr == m_imageListCtrl.m_hWnd )
		return;		// dialog not initialized

	UpdateData();
	m_imageListCtrl.Invalidate();
}
