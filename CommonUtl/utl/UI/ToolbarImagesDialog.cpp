
#include "pch.h"
#include "ToolbarImagesDialog.h"
#include "ImageCommandLookup.h"
#include "Image_fwd.h"
#include "ImageStore.h"
#include "Icon.h"
#include "ControlBar_fwd.h"
#include "ResizeFrameStatic.h"
#include "SampleView.h"
#include "WndUtils.h"
#include "resource.h"
#include "utl/AppTools.h"
#include "utl/ContainerOwnership.h"
#include "utl/StringUtilities.h"
#include "utl/Subject.h"
#include <afxtoolbar.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#include "ReportListControl.hxx"


namespace reg
{
	static const TCHAR section_dialog[] = _T("utl\\ToolbarImagesDialog");
	static const TCHAR section_MfcPage[] = _T("utl\\ToolbarImagesDialog\\MFC Page");
	static const TCHAR section_MfcPageSplitterH[] = _T("utl\\ToolbarImagesDialog\\MFC Page\\SplitterH");
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

CToolbarImagesDialog::CToolbarImagesDialog( CWnd* pParentWnd )
	: CLayoutDialog( IDD_IMAGES_TOOLBAR_DIALOG, pParentWnd )
{
	m_regSection = m_childSheet.m_regSection = reg::section_dialog;
	RegisterCtrlLayout( ARRAY_SPAN( layout::s_styles ) );
	LoadDlgIcon( ID_EDIT_LIST_ITEMS );

	m_childSheet.AddPage( new CToolbarImagesPage( CMFCToolBar::GetImages() ) );
	m_childSheet.AddPage( new CIconImagesPage() );
}

CToolbarImagesDialog::~CToolbarImagesDialog()
{
}

mfc::TRuntimeClassList* CToolbarImagesDialog::GetCustomPages( void )
{
	static mfc::TRuntimeClassList s_pagesList;

	if ( s_pagesList.IsEmpty() )
	{
		s_pagesList.AddTail( RUNTIME_CLASS( CToolbarImagesPage ) );
		s_pagesList.AddTail( RUNTIME_CLASS( CIconImagesPage ) );
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

struct CBaseImageItem : public TBasicSubject
{
	CBaseImageItem( int index, UINT cmdId, const std::tstring* pCmdName = nullptr, const std::tstring* pCmdLiteral = nullptr )
		: m_index( index )
		, m_cmdId( cmdId )
		, m_imageSize( 0, 0 )
	{
		if ( pCmdName != nullptr )
			m_cmdName = *pCmdName;

		if ( pCmdLiteral != nullptr )
			m_cmdLiteral = *pCmdLiteral;
	}

	virtual const std::tstring& GetCode( void ) const implement { return m_cmdName; }
	virtual std::tstring GetDisplayCode( void ) const { return GetCode(); }

	virtual bool Draw( CDC* pDC, const CRect& itemImageRect, bool drawDisabled = false, BYTE alphaSrc = 255 ) const = 0;
	virtual bool RenderSample( CDC* pDC, const CRect& normalRect, const CRect& largeRect, bool drawDisabled = false, BYTE alphaSrc = 255 ) const = 0;
public:
	int m_index;
	UINT m_cmdId;
	std::tstring m_cmdName;
	std::tstring m_cmdLiteral;

	CSize m_imageSize;
};


// CToolbarImageItem class

struct CToolbarImageItem : public CBaseImageItem
{
	CToolbarImageItem( CMFCToolBarImages* pImages, int index, UINT cmdId, const std::tstring* pCmdName = nullptr, const std::tstring* pCmdLiteral = nullptr )
		: CBaseImageItem( index, cmdId, pCmdName, pCmdLiteral )
		, m_pImages( pImages )
	{
		ASSERT_PTR( m_pImages );

		m_imageSize = m_pImages->GetImageSize();
	}


	// base overrides

	virtual bool Draw( CDC* pDC, const CRect& itemImageRect, bool drawDisabled = false, BYTE alphaSrc = 255 ) const
	{
		CAfxDrawState drawState;

		if ( !m_pImages->PrepareDrawImage( drawState ) )
			return false;

		CRect imageRect( 0, 0, m_imageSize.cx, m_imageSize.cy );

		ui::CenterRect( imageRect, itemImageRect );

		m_pImages->Draw( pDC, imageRect.left, imageRect.top, m_index, FALSE, drawDisabled, FALSE, FALSE, FALSE, alphaSrc );
		m_pImages->EndDrawImage( drawState );
		return true;
	}

	virtual bool RenderSample( CDC* pDC, const CRect& normalRect, const CRect& largeRect, bool drawDisabled = false, BYTE alphaSrc = 255 ) const
	{
		CAfxDrawState drawState;

		if ( !m_pImages->PrepareDrawImage( drawState ) )
			return false;

		m_pImages->Draw( pDC, normalRect.left, normalRect.top, m_index, FALSE, drawDisabled, FALSE, FALSE, FALSE, alphaSrc );
		mfc::ToolBarImages_DrawStretch( m_pImages, pDC, largeRect, m_index, false, drawDisabled, false, false, false, alphaSrc );

		m_pImages->EndDrawImage( drawState );
		return true;
	}
private:
	CMFCToolBarImages* m_pImages;
};


// CIconImageItem class

struct CIconImageItem : public CBaseImageItem
{
	CIconImageItem( const CIcon* pIcon, int index, UINT cmdId, const std::tstring* pCmdName = nullptr, const std::tstring* pCmdLiteral = nullptr )
		: CBaseImageItem( index, cmdId, pCmdName, pCmdLiteral )
		, m_pIcon( pIcon )
	{
		ASSERT_PTR( m_pIcon );

		m_imageSize = m_pIcon->GetSize();
	}


	// base overrides

	virtual bool Draw( CDC* pDC, const CRect& itemImageRect, bool drawDisabled = false, BYTE alphaSrc = 255 ) const
	{
		alphaSrc;
		CRect imageRect( 0, 0, m_imageSize.cx, m_imageSize.cy );

		ui::CenterRect( imageRect, itemImageRect );

		m_pIcon->DrawStretch( *pDC, imageRect, !drawDisabled );
		return true;
	}

	virtual bool RenderSample( CDC* pDC, const CRect& normalRect, const CRect& largeRect, bool drawDisabled = false, BYTE alphaSrc = 255 ) const
	{
		alphaSrc;
		m_pIcon->DrawStretch( *pDC, normalRect, !drawDisabled );
		m_pIcon->DrawStretch( *pDC, largeRect, !drawDisabled );
		return true;
	}
private:
	const CIcon* m_pIcon;
};


// CToolbarImagesPage implementation

IMPLEMENT_DYNAMIC( CBaseImagesPage, CPropertyPage )

CBaseImagesPage::CBaseImagesPage( UINT templateId, const CLayoutStyle layoutStyles[], unsigned int count )
	: CLayoutPropertyPage( templateId )
	, m_imageBoundsSize( CIconSize::GetSizeOf( SmallIcon ) )
	, m_selItemIndex( AfxGetApp()->GetProfileInt( reg::section_MfcPage, reg::entry_SelItemIndex, 0 ) )
	, m_drawDisabled( AfxGetApp()->GetProfileInt( reg::section_MfcPage, reg::entry_DrawDisabled, false ) != FALSE )
	, m_alphaSrc( 255u )
	, m_imageListCtrl( IDC_TOOLBAR_IMAGES_LIST )
{
	SetUseLazyUpdateData();			// call UpdateData on page activation change

	m_imageListCtrl.SetSection( reg::section_MfcPage );
	m_imageListCtrl.SetCustomIconDraw( this );
	m_imageListCtrl.SetUseAlternateRowColoring();
	m_imageListCtrl.SetCommandFrame();			// handle ID_EDIT_COPY in this page
	m_imageListCtrl.SetTabularTextSep();		// tab-separated multi-column copy

	m_pSampleView.reset( new CSampleView( this ) );
	m_pSampleView->SetBorderColor( CLR_DEFAULT );

	m_pBottomLayoutStatic.reset( new CLayoutStatic() );
	m_pBottomLayoutStatic->RegisterCtrlLayout( layoutStyles, count );

	m_pHorizSplitterFrame.reset( new CResizeFrameStatic( &m_imageListCtrl, m_pBottomLayoutStatic.get(), resize::NorthSouth ) );
	m_pHorizSplitterFrame->SetSection( reg::section_MfcPageSplitterH );
	m_pHorizSplitterFrame->GetGripBar()
		.SetMinExtents( 50, 16 )
		.SetFirstExtentPercentage( 75 );
}

CBaseImagesPage::~CBaseImagesPage()
{
	utl::ClearOwningContainer( m_imageItems );
}

void CBaseImagesPage::OutputList( void )
{
	CScopedInternalChange internalChange( &m_imageListCtrl );

	m_imageListCtrl.DeleteAllItems();

	for ( UINT i = 0; i != m_imageItems.size(); ++i )
	{
		CBaseImageItem* pImageItem = m_imageItems[ i ];

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

CSize CBaseImagesPage::GetItemImageSize( ui::GlyphGauge glyphGauge ) const implements(ui::ICustomImageDraw)
{
	switch ( glyphGauge )
	{
		default: ASSERT( false );
		case ui::SmallGlyph:	return CIconSize::GetSizeOf( SmallIcon );
		case ui::LargeGlyph:	return CIconSize::GetSizeOf( LargeIcon );
	}
}

bool CBaseImagesPage::SetItemImageSize( const CSize& imageBoundsSize ) implements(ui::ICustomImageDraw)
{
	if ( m_imageBoundsSize == imageBoundsSize )
		return false;

	m_imageBoundsSize = imageBoundsSize;
	return true;
}

bool CBaseImagesPage::DrawItemImage( CDC* pDC, const utl::ISubject* pSubject, const CRect& itemImageRect ) implements(ui::ICustomImageDraw)
{
	const CBaseImageItem* pImageItem = checked_static_cast<const CBaseImageItem*>( pSubject );

	return pImageItem->Draw( pDC, itemImageRect, m_drawDisabled, m_alphaSrc );
}

bool CBaseImagesPage::RenderSample( CDC* pDC, const CRect& boundsRect ) implements(ui::ISampleCallback)
{
	GetGlobalData()->DrawParentBackground( m_pSampleView.get(), pDC, const_cast<CRect*>( &boundsRect ) );

	if ( CBaseImageItem* pImageItem = m_imageListCtrl.GetCaretAs<CBaseImageItem>() )
	{
		enum { NormalEdge = 10 };

		CRect normalBoundsRect = boundsRect, largeBoundsRect = boundsRect;

		normalBoundsRect.right = normalBoundsRect.left + pImageItem->m_imageSize.cx + NormalEdge * 2;
		largeBoundsRect.left = normalBoundsRect.right;

		int zoomSize = std::min( largeBoundsRect.Width(), largeBoundsRect.Height() );

		CRect normalRect( 0, 0, pImageItem->m_imageSize.cx, pImageItem->m_imageSize.cy );
		CRect largeRect( 0, 0, zoomSize, zoomSize );

		ui::CenterRect( normalRect, normalBoundsRect );
		ui::CenterRect( largeRect, largeBoundsRect );

		if ( !pImageItem->RenderSample( pDC, normalRect, largeRect, m_drawDisabled, m_alphaSrc ) )
			return false;
	}

	return true;
}

void CBaseImagesPage::DoDataExchange( CDataExchange* pDX )
{
	//bool firstInit = nullptr == m_imageListCtrl.m_hWnd;

	DDX_Control( pDX, IDC_TOOLBAR_IMAGES_LIST, m_imageListCtrl );
	m_pSampleView->DDX_Placeholder( pDX, IDC_TOOLBAR_IMAGE_SAMPLE );

	ui::DDX_Bool( pDX, IDC_DRAW_DISABLED_CHECK, m_drawDisabled );
	DDX_Control( pDX, IDC_LAYOUT_FRAME_1_STATIC, *m_pBottomLayoutStatic );
	DDX_Control( pDX, IDC_HORIZ_SPLITTER_STATIC, *m_pHorizSplitterFrame );

	__super::DoDataExchange( pDX );
}

BEGIN_MESSAGE_MAP( CBaseImagesPage, CLayoutPropertyPage )
	ON_WM_DESTROY()
	ON_NOTIFY( LVN_ITEMCHANGED, IDC_TOOLBAR_IMAGES_LIST, OnLvnItemChanged_ImageListCtrl )
	ON_BN_CLICKED( IDC_DRAW_DISABLED_CHECK, OnRedrawImagesList )
	ON_EN_CHANGE( IDC_ALPHA_SRC_EDIT, OnRedrawImagesList )
	ON_COMMAND( ID_EDIT_COPY, OnCopyItems )
	ON_UPDATE_COMMAND_UI( ID_EDIT_COPY, OnUpdateCopyItems )
END_MESSAGE_MAP()

void CBaseImagesPage::OnDestroy( void )
{
	AfxGetApp()->WriteProfileInt( reg::section_MfcPage, reg::entry_SelItemIndex, m_selItemIndex );
	AfxGetApp()->WriteProfileInt( reg::section_MfcPage, reg::entry_DrawDisabled, m_drawDisabled );

	__super::OnDestroy();
}

void CBaseImagesPage::OnCopyItems( void )
{
	m_imageListCtrl.Copy( ds::Indexes | ds::ItemsText );
}

void CBaseImagesPage::OnUpdateCopyItems( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( m_imageListCtrl.AnySelected() );
}

void CBaseImagesPage::OnLvnItemChanged_ImageListCtrl( NMHDR* pNmHdr, LRESULT* pResult )
{
	NMLISTVIEW* pNmList = (NMLISTVIEW*)pNmHdr;
	*pResult = 0;

	if ( CReportListControl::IsSelectionChangeNotify( pNmList, LVIS_SELECTED | LVIS_FOCUSED ) )
	{
		m_selItemIndex = m_imageListCtrl.GetCurSel();
		m_pSampleView->Invalidate();
	}
}

void CBaseImagesPage::OnRedrawImagesList( void )
{
	if ( nullptr == m_imageListCtrl.m_hWnd )
		return;		// dialog not initialized

	UpdateData( DialogSaveChanges );

	m_imageListCtrl.Invalidate();
	m_pSampleView->Invalidate();
}


// CToolbarImagesPage implementation

namespace layout
{
	static CLayoutStyle s_mfcPageStyles[] =
	{
		{ IDC_IMAGE_COUNT_STATIC, SizeX },
		{ IDC_HORIZ_SPLITTER_STATIC, Size },
		//{ IDC_TOOLBAR_IMAGES_LIST, Size },

	};

	static CLayoutStyle s_bottomFrameStyles[] =
	{
		{ IDC_TOOLBAR_IMAGE_SAMPLE, Size },
		{ IDC_GROUP_BOX_1, SizeY },
		{ IDC_ALPHA_SRC_LABEL, OffsetOrigin },			// use to offset controls in frame layout (non-master layout)
		{ IDC_DRAW_DISABLED_CHECK, OffsetOrigin },
		{ IDC_ALPHA_SRC_EDIT, OffsetOrigin },
		{ IDC_ALPHA_SRC_SPIN, OffsetOrigin }
	};
}

IMPLEMENT_DYNCREATE( CToolbarImagesPage, CBaseImagesPage )

CToolbarImagesPage::CToolbarImagesPage( CMFCToolBarImages* pImages /*= nullptr*/ )
	: CBaseImagesPage( IDD_IMAGES_TOOLBAR_PAGE, ARRAY_SPAN( layout::s_bottomFrameStyles ) )
	, m_pImages( pImages != nullptr ? pImages : CMFCToolBar::GetImages() )
{
	RegisterCtrlLayout( ARRAY_SPAN( layout::s_mfcPageStyles ) );
	ASSERT_PTR( m_pImages );

	m_alphaSrc = static_cast<BYTE>( AfxGetApp()->GetProfileInt( reg::section_MfcPage, reg::entry_AlphaSrc, 255u ) );

	UINT imageCount = m_pImages->GetCount();
	const mfc::CImageCommandLookup* pImageCmds = mfc::CImageCommandLookup::Instance();

	m_imageItems.reserve( imageCount );
	for ( UINT index = 0; index != imageCount; ++index )
	{
		UINT cmdId = pImageCmds->FindCommand( index );
		const std::tstring* pCmdName = pImageCmds->FindCommandName( cmdId );
		const std::tstring* pCmdLiteral = pImageCmds->FindCommandLiteral( cmdId );

		m_imageItems.push_back( new CToolbarImageItem( m_pImages, index, cmdId, pCmdName, pCmdLiteral ) );
	}
}

CToolbarImagesPage::~CToolbarImagesPage()
{
}

void CToolbarImagesPage::DoDataExchange( CDataExchange* pDX )
{
	bool firstInit = nullptr == m_imageListCtrl.m_hWnd;
	static std::tstring s_imageCountFmt = ui::GetDlgItemText( this, IDC_IMAGE_COUNT_STATIC );

	ui::DDX_Number( pDX, IDC_ALPHA_SRC_EDIT, m_alphaSrc );

	if ( firstInit )
	{
		CSpinButtonCtrl* pAlphaSrcSpinBtn = ui::GetDlgItemAs<CSpinButtonCtrl>( this, IDC_ALPHA_SRC_SPIN );

		ASSERT_PTR( pAlphaSrcSpinBtn->GetSafeHwnd() );
		pAlphaSrcSpinBtn->SetRange32( 0, 255 );
	}

	__super::DoDataExchange( pDX );

	static const UINT s_alphaCtrls[] = { IDC_ALPHA_SRC_LABEL, IDC_ALPHA_SRC_EDIT };
	ui::EnableControls( m_hWnd, ARRAY_SPAN( s_alphaCtrls ), !m_drawDisabled );

	if ( DialogOutput == pDX->m_bSaveAndValidate )
	{
		ui::SetDlgItemText( this, IDC_IMAGE_COUNT_STATIC, str::Format( s_imageCountFmt.c_str(), m_pImages->GetCount() ) );
		OutputList();
	}
}

BEGIN_MESSAGE_MAP( CToolbarImagesPage, CBaseImagesPage )
	ON_WM_DESTROY()
END_MESSAGE_MAP()

void CToolbarImagesPage::OnDestroy( void )
{
	AfxGetApp()->WriteProfileInt( reg::section_MfcPage, reg::entry_AlphaSrc, m_alphaSrc );

	__super::OnDestroy();
}


// CIconImagesPage implementation

namespace layout
{
	static CLayoutStyle s_iconPageStyles[] =
	{
		{ IDC_IMAGE_COUNT_STATIC, SizeX },
		{ IDC_HORIZ_SPLITTER_STATIC, Size },
		//{ IDC_TOOLBAR_IMAGES_LIST, Size },

	};

	static CLayoutStyle s_iconBottomFrameStyles[] =
	{
		{ IDC_TOOLBAR_IMAGE_SAMPLE, Size },
		{ IDC_GROUP_BOX_1, SizeY },
		{ IDC_DRAW_DISABLED_CHECK, OffsetOrigin }
	};
}

IMPLEMENT_DYNCREATE( CIconImagesPage, CBaseImagesPage )

CIconImagesPage::CIconImagesPage( ui::IImageStore* pImageStore /*= nullptr*/ )
	: CBaseImagesPage( IDD_IMAGES_ICONS_PAGE, ARRAY_SPAN( layout::s_iconBottomFrameStyles ) )
	, m_pImageStore( pImageStore != nullptr ? pImageStore : app::GetSharedImageStore() )
{
	RegisterCtrlLayout( ARRAY_SPAN( layout::s_iconPageStyles ) );
	ASSERT_PTR( m_pImageStore );

	UINT imageCount = CMFCToolBar::GetImages()->GetCount();
	const mfc::CImageCommandLookup* pImageCmds = mfc::CImageCommandLookup::Instance();

	m_imageItems.reserve( imageCount );
	for ( UINT index = 0; index != imageCount; ++index )
	{
		UINT cmdId = pImageCmds->FindCommand( index );
		if ( const CIcon* pIcon = m_pImageStore->RetrieveIcon( CIconId( cmdId, SmallIcon ) ) )
		{
			const std::tstring* pCmdName = pImageCmds->FindCommandName( cmdId );
			const std::tstring* pCmdLiteral = pImageCmds->FindCommandLiteral( cmdId );

			m_imageItems.push_back( new CIconImageItem( pIcon, index, cmdId, pCmdName, pCmdLiteral ) );
		}
	}
}

CIconImagesPage::~CIconImagesPage()
{
}

void CIconImagesPage::DoDataExchange( CDataExchange* pDX )
{
	//bool firstInit = nullptr == m_imageListCtrl.m_hWnd;
	static std::tstring s_imageCountFmt = ui::GetDlgItemText( this, IDC_IMAGE_COUNT_STATIC );

	__super::DoDataExchange( pDX );

	static const UINT s_alphaCtrls[] = { IDC_ALPHA_SRC_LABEL, IDC_ALPHA_SRC_EDIT };
	ui::EnableControls( m_hWnd, ARRAY_SPAN( s_alphaCtrls ), !m_drawDisabled );

	if ( DialogOutput == pDX->m_bSaveAndValidate )
	{
		ui::SetDlgItemText( this, IDC_IMAGE_COUNT_STATIC, str::Format( s_imageCountFmt.c_str(), m_imageItems.size() ) );
		OutputList();
	}
}

BEGIN_MESSAGE_MAP( CIconImagesPage, CBaseImagesPage )
	ON_WM_DESTROY()
END_MESSAGE_MAP()

void CIconImagesPage::OnDestroy( void )
{
	//AfxGetApp()->WriteProfileInt( reg::section_MfcPage, reg::entry_AlphaSrc, m_alphaSrc );

	__super::OnDestroy();
}
