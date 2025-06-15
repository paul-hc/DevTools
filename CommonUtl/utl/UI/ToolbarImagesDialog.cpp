
#include "pch.h"
#include "ToolbarImagesDialog.h"
#include "ImageCommandLookup.h"
#include "Image_fwd.h"
#include "ImageStore_fwd.h"
#include "Icon.h"
#include "ControlBar_fwd.h"
#include "CmdTagStore.h"
#include "CmdUpdate.h"
#include "ResizeFrameStatic.h"
#include "SampleView.h"
#include "WndUtilsEx.h"
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
	static const TCHAR* s_pageTitle[] = { _T("ToolBar Images"), _T("Store Toolbar Images"), _T("Icons") };

	static const TCHAR section_dialog[] = _T("utl\\ToolbarImagesDialog");
	static const TCHAR section_dialogSplitterH[] = _T("utl\\ToolbarImagesDialog\\SplitterH");
	static const TCHAR entry_SelItemIndex[] = _T("SelItemIndex");
	static const TCHAR entry_DrawDisabled[] = _T("DrawDisabled");
	static const TCHAR entry_SrcAlpha[] = _T("SrcAlpha");
}

namespace layout
{
	static CLayoutStyle s_styles[] =
	{
		{ IDC_HORIZ_SPLITTER_STATIC, Size },			// frame of the splitter (containing all controls, except dialog button)
		{ IDOK, Move },
		{ IDCANCEL, Move }
	};

	static CLayoutStyle s_splitBottomFrameStyles[] =
	{
		{ IDC_GROUP_BOX_1, SizeY },
		{ IDC_DRAW_DISABLED_CHECK, OffsetOrigin },
		{ IDC_ALPHA_SRC_LABEL, OffsetOrigin },			// use to offset controls in frame layout (non-master layout)
		{ IDC_ALPHA_SRC_EDIT, OffsetOrigin },
		{ IDC_ALPHA_SRC_SPIN, OffsetOrigin },
		{ IDC_ALPHA_DISABLED_LABEL, OffsetOrigin },
		{ IDC_ALPHA_DISABLED_EDIT, OffsetOrigin },
		{ IDC_ALPHA_DISABLED_SPIN, OffsetOrigin },
		{ IDC_TOOLBAR_IMAGE_SAMPLE, Size }
	};
}

CToolbarImagesDialog::CToolbarImagesDialog( CWnd* pParentWnd )
	: CLayoutDialog( IDD_IMAGES_TOOLBAR_DIALOG, pParentWnd )
{
	m_regSection = m_childSheet.m_regSection = reg::section_dialog;
	RegisterCtrlLayout( ARRAY_SPAN( layout::s_styles ) );
	LoadDlgIcon( ID_EDIT_LIST_ITEMS );

	m_childSheet.AddPage( new CMfcToolbarImagesPage( CMFCToolBar::GetImages() ) );
	m_childSheet.AddPage( new CStoreToolbarImagesPage() );
	m_childSheet.AddPage( new CIconImagesPage() );

	m_pSampleView.reset( new CSampleView( this ) );
	//m_pSampleView->SetBorderColor( color::Red );		// for debugging
	m_pSampleView->SetBkColor( ::GetSysColor( COLOR_BTNFACE ) );

	// IDC_LAYOUT_FRAME_1_STATIC:
	m_pSplitBottomLayoutStatic.reset( new CLayoutStatic() );
	m_pSplitBottomLayoutStatic->RegisterCtrlLayout( ARRAY_SPAN( layout::s_splitBottomFrameStyles ) );
	m_pSplitBottomLayoutStatic->SetUseSmoothTransparentGroups();

	// IDC_HORIZ_SPLITTER_STATIC:
	m_pUpDownSplitterFrame.reset( new CResizeFrameStatic( &m_childSheet, m_pSplitBottomLayoutStatic.get(), resize::NorthSouth /*, resize::ToggleFirst*/ ) );
	m_pUpDownSplitterFrame->SetSection( reg::section_dialogSplitterH );
	m_pUpDownSplitterFrame->GetGripBar()
		.SetMinExtents( 100, 90 )			// correlate with dialog minimum heights from the resource template
		.SetFirstExtentPercentage( 75 )
		.SetPaneSpacing( 5, 5 );

	m_sharedData.m_drawDisabled = AfxGetApp()->GetProfileInt( reg::section_dialog, reg::entry_DrawDisabled, false ) != FALSE;
	m_sharedData.m_srcAlpha = static_cast<BYTE>( AfxGetApp()->GetProfileInt( reg::section_dialog, reg::entry_SrcAlpha, 255u ) );
	m_sharedData.m_disabledAlpha = mfc::ToolBarImages_RefDisabledImageAlpha();
	m_sharedData.m_pSampleView = m_pSampleView.get();
}

CToolbarImagesDialog::~CToolbarImagesDialog()
{
}

mfc::TRuntimeClassList* CToolbarImagesDialog::GetCustomPages( void )
{
	static mfc::TRuntimeClassList s_pagesList;

	if ( s_pagesList.IsEmpty() )
	{
		s_pagesList.AddTail( RUNTIME_CLASS( CMfcToolbarImagesPage ) );
		s_pagesList.AddTail( RUNTIME_CLASS( CIconImagesPage ) );
	}

	return &s_pagesList;
}

CBaseImagesPage* CToolbarImagesDialog::GetActiveChildPage( void ) const
{
	return checked_static_cast<CBaseImagesPage*>( m_childSheet.GetActivePage() );
}

bool CToolbarImagesDialog::RenderSample( CDC* pDC, const CRect& boundsRect ) implements( ui::ISampleCallback )
{
	//GetGlobalData()->DrawParentBackground( m_pSampleView.get(), pDC, const_cast<CRect*>( &boundsRect ));		/* doesn't work with double buffering, leaves background black */

	return GetActiveChildPage()->RenderImageSample( pDC, boundsRect );
}

void CToolbarImagesDialog::OnIdleUpdateControls( void ) overrides(CLayoutDialog)
{
	__super::OnIdleUpdateControls();

	static const UINT s_ctrlIds[] = { IDC_ALPHA_SRC_LABEL, IDC_ALPHA_SRC_EDIT, IDC_ALPHA_DISABLED_LABEL, IDC_ALPHA_DISABLED_EDIT };
	ui::UpdateDlgControlsUI( m_hWnd, ARRAY_SPAN( s_ctrlIds ), this );
}

void CToolbarImagesDialog::DoDataExchange( CDataExchange* pDX )
{
	bool firstInit = nullptr == m_childSheet.m_hWnd;

	if ( firstInit )
	{
		ui::SetSpinRange( this, IDC_ALPHA_SRC_SPIN, 0, 255 );
		ui::SetSpinRange( this, IDC_ALPHA_DISABLED_SPIN, 0, 255 );
	}

	m_pSampleView->DDX_Placeholder( pDX, IDC_TOOLBAR_IMAGE_SAMPLE );
	m_childSheet.DDX_DetailSheet( pDX, IDC_ITEMS_SHEET );

	ui::DDX_Bool( pDX, IDC_DRAW_DISABLED_CHECK, m_sharedData.m_drawDisabled );
	ui::DDX_Number( pDX, IDC_ALPHA_SRC_EDIT, m_sharedData.m_srcAlpha );
	ui::DDX_Number( pDX, IDC_ALPHA_DISABLED_EDIT, m_sharedData.m_disabledAlpha );

	DDX_Control( pDX, IDC_LAYOUT_FRAME_1_STATIC, *m_pSplitBottomLayoutStatic );
	DDX_Control( pDX, IDC_HORIZ_SPLITTER_STATIC, *m_pUpDownSplitterFrame );

	if ( DialogOutput == pDX->m_bSaveAndValidate )
	{
//m_childSheet.ModifyStyle( 0, WS_BORDER );		// dbg
		m_childSheet.OutputPages();
	}
	else
		mfc::ToolBarImages_RefDisabledImageAlpha() = m_sharedData.m_disabledAlpha;		// modify the global field!

	__super::DoDataExchange( pDX );
}


// message handlers

BEGIN_MESSAGE_MAP( CToolbarImagesDialog, CLayoutDialog )
	ON_WM_DESTROY()
	ON_BN_CLICKED( IDC_DRAW_DISABLED_CHECK, OnRedrawImagesList )
	ON_EN_CHANGE( IDC_ALPHA_SRC_EDIT, OnRedrawImagesList )
	ON_EN_CHANGE( IDC_ALPHA_DISABLED_EDIT, OnRedrawImagesList )
	ON_UPDATE_COMMAND_UI_RANGE( IDC_ALPHA_SRC_LABEL, IDC_ALPHA_DISABLED_SPIN, OnUpdateUseAlpha )
END_MESSAGE_MAP()

void CToolbarImagesDialog::OnDestroy( void )
{
	AfxGetApp()->WriteProfileInt( reg::section_dialog, reg::entry_DrawDisabled, m_sharedData.m_drawDisabled );
	AfxGetApp()->WriteProfileInt( reg::section_dialog, reg::entry_SrcAlpha, m_sharedData.m_srcAlpha );

	__super::OnDestroy();
}

void CToolbarImagesDialog::OnRedrawImagesList( void )
{
	if ( m_childSheet.m_hWnd != nullptr )		// dialog initialized?
	{
		UpdateData( DialogSaveChanges );
		GetActiveChildPage()->UpdateData( DialogSaveChanges );
	}
}

void CToolbarImagesDialog::OnUpdateUseAlpha( CCmdUI* pCmdUI )
{
	switch ( m_childSheet.GetActiveIndex() )
	{
		case MfcToolBarImagesPage:
		case StoreToolbarImagesPage:
			pCmdUI->Enable( CMFCToolBar::GetImages()->GetCount() != 0 );
			break;
		default:
			pCmdUI->Enable( false );
	}
}


// CBaseImageItem class

struct CBaseImageItem : public TBasicSubject
{
	CBaseImageItem( int index, UINT cmdId, const std::tstring* pCmdName = nullptr, const std::tstring* pCmdLiteral = nullptr )
		: m_index( index )
		, m_cmdId( cmdId )
		, m_imageSize( 0, 0 )
		, m_bitsPerPixel( ILC_COLOR32 )
		, m_indexText( num::FormatNumber( m_index ) )
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
	TBitsPerPixel m_bitsPerPixel;
	std::tstring m_indexText;
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
		m_bitsPerPixel = static_cast<TBitsPerPixel>( mfc::ToolBarImages_GetBitsPerPixel( m_pImages ) );
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

		ui::CIconEntry iconEntry = pIcon->GetGroupIconEntry();

		m_imageSize = iconEntry.GetSize();
		m_bitsPerPixel = iconEntry.m_bitsPerPixel;
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


// CBaseImagesPage implementation

IMPLEMENT_DYNAMIC( CBaseImagesPage, CPropertyPage )

CBaseImagesPage::CBaseImagesPage( UINT templateId, const TCHAR* pTitle )
	: CLayoutPropertyPage( templateId )
	, m_pDlgData( nullptr )
	, m_regSection( path::Combine( reg::section_dialog, pTitle ) )
	, m_imageBoundsSize( CIconSize::GetSizeOf( SmallIcon ) )
	, m_selItemIndex( AfxGetApp()->GetProfileInt( m_regSection.c_str(), reg::entry_SelItemIndex, 0 ) )
	, m_imageListCtrl( IDC_TOOLBAR_IMAGES_LIST )
{
	SetUseLazyUpdateData();		// call UpdateData on page activation change
	SetTitle( pTitle );			// override resource-based page title

	m_imageListCtrl.SetSection( path::Combine( m_regSection.c_str(), _T("ImageList") ) );	// page-specific sub-section
	m_imageListCtrl.SetCustomIconDraw( this );
	m_imageListCtrl.SetUseAlternateRowColoring();
	m_imageListCtrl.SetCommandFrame();			// handle ID_EDIT_COPY in this page
	m_imageListCtrl.SetTabularTextSep();		// tab-separated multi-column copy
}

CBaseImagesPage::~CBaseImagesPage()
{
	utl::ClearOwningContainer( m_imageItems );
}

void CBaseImagesPage::OutputList( void )
{
	CScopedInternalChange internalChange( &m_imageListCtrl );

	m_imageListCtrl.DeleteAllItems();
	AddListItems();
	m_imageListCtrl.InitialSortList();		// store original order and sort by current criteria

	if ( m_selItemIndex != -1 )
	{
		m_imageListCtrl.SetCurSel( m_selItemIndex );
		m_imageListCtrl.EnsureVisible( m_selItemIndex, false );
	}
}

void CBaseImagesPage::AddListItems( void )
{
	for ( UINT i = 0; i != m_imageItems.size(); ++i )
		AddListItem( i, m_imageItems[ i ] );
}

void CBaseImagesPage::AddListItem( int itemPos, const CBaseImageItem* pImageItem )
{
	ASSERT_PTR( pImageItem );

	m_imageListCtrl.InsertObjectItem( itemPos, pImageItem );
	m_imageListCtrl.SetSubItemText( itemPos, Index, pImageItem->m_indexText );
	m_imageListCtrl.SetSubItemText( itemPos, Format, str::Format( _T("%dx%d, %d-bit"), pImageItem->m_imageSize.cx, pImageItem->m_imageSize.cy, pImageItem->m_bitsPerPixel ) );
	m_imageListCtrl.SetSubItemText( itemPos, CmdId, pImageItem->m_cmdId != 0 ? str::Format( _T("%d, 0x%04X"), pImageItem->m_cmdId, pImageItem->m_cmdId ) : num::FormatNumber( pImageItem->m_cmdId ) );
	m_imageListCtrl.SetSubItemText( itemPos, CmdLiteral, pImageItem->m_cmdLiteral );
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

	return pImageItem->Draw( pDC, itemImageRect, m_pDlgData->m_drawDisabled, m_pDlgData->m_srcAlpha );
}

bool CBaseImagesPage::RenderImageSample( CDC* pDC, const CRect& boundsRect ) implements(ui::ISampleCallback)
{
	if ( CBaseImageItem* pImageItem = m_imageListCtrl.GetCaretAs<CBaseImageItem>() )
	{
		enum { NormalEdge = 10 };

		CRect normalBoundsRect = boundsRect, largeBoundsRect = boundsRect;

		normalBoundsRect.right = normalBoundsRect.left + pImageItem->m_imageSize.cx + NormalEdge * 2;
		largeBoundsRect.left = normalBoundsRect.right + NormalEdge * 2;

		int zoomSize = std::min( largeBoundsRect.Width(), largeBoundsRect.Height() );

		CRect normalRect( 0, 0, pImageItem->m_imageSize.cx, pImageItem->m_imageSize.cy );
		CRect largeRect( 0, 0, zoomSize, zoomSize );

		ui::CenterRect( normalRect, normalBoundsRect );
		ui::CenterRect( largeRect, largeBoundsRect );

		if ( !pImageItem->RenderSample( pDC, normalRect, largeRect, m_pDlgData->m_drawDisabled, m_pDlgData->m_srcAlpha ) )
			return false;
	}

	return true;
}

void CBaseImagesPage::DoDataExchange( CDataExchange* pDX )
{
	bool firstInit = nullptr == m_imageListCtrl.m_hWnd;

	if ( firstInit )
		m_pDlgData = &checked_static_cast<CToolbarImagesDialog*>( GetParent()->GetParent() )->m_sharedData;

	DDX_Control( pDX, IDC_TOOLBAR_IMAGES_LIST, m_imageListCtrl );

	if ( DialogOutput == pDX->m_bSaveAndValidate )
	{
		if ( firstInit )
		{
			ui::SetDlgItemText( this, IDC_IMAGE_COUNT_STATIC, m_imageCountText );
			OutputList();
		}
	}

	m_imageListCtrl.Invalidate();
	m_pDlgData->m_pSampleView->SafeRedraw();

	__super::DoDataExchange( pDX );
}

BEGIN_MESSAGE_MAP( CBaseImagesPage, CLayoutPropertyPage )
	ON_WM_DESTROY()
	ON_NOTIFY( LVN_ITEMCHANGED, IDC_TOOLBAR_IMAGES_LIST, OnLvnItemChanged_ImageListCtrl )
	ON_COMMAND( ID_EDIT_COPY, OnCopyItems )
	ON_UPDATE_COMMAND_UI( ID_EDIT_COPY, OnUpdateCopyItems )
END_MESSAGE_MAP()

void CBaseImagesPage::OnDestroy( void )
{
	AfxGetApp()->WriteProfileInt( m_regSection.c_str(), reg::entry_SelItemIndex, m_selItemIndex );

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
		m_pDlgData->m_pSampleView->Invalidate();
	}
}


// CMfcToolbarImagesPage implementation

namespace layout
{
	static CLayoutStyle s_basePageStyles[] =
	{
		{ IDC_IMAGE_COUNT_STATIC, SizeX },
		{ IDC_TOOLBAR_IMAGES_LIST, Size }
	};
}

IMPLEMENT_DYNCREATE( CMfcToolbarImagesPage, CBaseImagesPage )

CMfcToolbarImagesPage::CMfcToolbarImagesPage( CMFCToolBarImages* pImages /*= nullptr*/ )
	: CBaseImagesPage( IDD_IMAGES_MFC_TOOLBAR_PAGE, reg::s_pageTitle[ CToolbarImagesDialog::MfcToolBarImagesPage ] )
	, m_pImages( pImages != nullptr ? pImages : CMFCToolBar::GetImages() )
{
	RegisterCtrlLayout( ARRAY_SPAN( layout::s_basePageStyles ) );
	ASSERT_PTR( m_pImages );

	UINT imageCount = m_pImages->GetCount();
	const mfc::CImageCommandLookup* pImageCmds = mfc::CImageCommandLookup::Instance();

	m_imageItems.reserve( imageCount );
	for ( UINT index = 0; index != imageCount; ++index )
	{
		UINT cmdId = pImageCmds->FindCommand( index );

		const std::tstring* pCmdName = pImageCmds->FindCommandName( cmdId );
		const std::tstring* pCmdLiteral = pImageCmds->FindCommandLiteral( cmdId );

		CBaseImageItem* pImageItem = new CToolbarImageItem( m_pImages, index, cmdId, pCmdName, pCmdLiteral );
		pImageItem->m_indexText = str::Format( _T("M-%d"), index );

		m_imageItems.push_back( pImageItem );
	}

	m_imageCountText = str::Format( _T("MFC Toolbars: total %d images"), m_imageItems.size() );
}

CMfcToolbarImagesPage::~CMfcToolbarImagesPage()
{
}


// CBaseStoreImagesPage implementation

IMPLEMENT_DYNAMIC( CBaseStoreImagesPage, CBaseImagesPage )

CBaseStoreImagesPage::CBaseStoreImagesPage( const TCHAR* pTitle, ui::IImageStore* pImageStore )
	: CBaseImagesPage( IDD_IMAGES_STORE_PAGE, pTitle )
	, m_pImageStore( pImageStore != nullptr ? pImageStore : ui::GetImageStoresSvc() )
{
	RegisterCtrlLayout( ARRAY_SPAN( layout::s_basePageStyles ) );
	ASSERT_PTR( m_pImageStore );
}

CBaseStoreImagesPage::~CBaseStoreImagesPage()
{
}


// CStoreToolbarImagesPage implementation

IMPLEMENT_DYNCREATE( CStoreToolbarImagesPage, CBaseStoreImagesPage )

CStoreToolbarImagesPage::CStoreToolbarImagesPage( ui::IImageStore* pImageStore /*= nullptr*/ )
	: CBaseStoreImagesPage( reg::s_pageTitle[ CToolbarImagesDialog::StoreToolbarImagesPage ], pImageStore != nullptr ? pImageStore : ui::GetImageStoresSvc() )
{
	std::vector<ui::CToolbarDescr*> toolbarDescrs;
	m_pImageStore->QueryToolbarDescriptors( toolbarDescrs );

	CMFCToolBarImages* pImages = CMFCToolBar::GetImages();
	bool hasMfcImageStore = GetCmdMgr() != nullptr && pImages != nullptr && pImages->GetCount() != 0;
	const mfc::CImageCommandLookup* pImageCmds = mfc::CImageCommandLookup::Instance();
	UINT totalImageCount = 0;

	for ( size_t toolbarPos = 0; toolbarPos != toolbarDescrs.size(); ++toolbarPos )
	{
		const ui::CToolbarDescr* pToolbarDescr = toolbarDescrs[ toolbarPos ];
		UINT toolbarId = pToolbarDescr->GetToolbarId();
		const std::tstring& toolbarTitle = pToolbarDescr->GetToolbarTitle();
		const std::vector<ui::CStripBtnInfo>& btnInfos = pToolbarDescr->GetBtnInfos();

		m_toolbarGroups.push_back( TToolbarGroupPair() );
		TToolbarGroupPair* pToolbarGroup = &m_toolbarGroups.back();

		pToolbarGroup->first = str::Format( _T("Toolbar #%d: ID=%d, 0x%04X"), toolbarPos + 1, toolbarId, toolbarId );
		if ( !toolbarTitle.empty() )
			pToolbarGroup->first += str::Format( _T(" \"%s\""), toolbarTitle.c_str() );

		for ( std::vector<ui::CStripBtnInfo>::const_iterator itBtnInfo = btnInfos.begin(); itBtnInfo != btnInfos.end(); ++itBtnInfo )
		{
			UINT cmdId = itBtnInfo->m_cmdId;
			const std::tstring* pCmdName = pImageCmds->FindCommandName( cmdId );
			const std::tstring* pCmdLiteral = pImageCmds->FindCommandLiteral( cmdId );

			int mfcImageIndex = hasMfcImageStore ? mfc::FindButtonImageIndex( itBtnInfo->m_cmdId ) : -1;

			CBaseImageItem* pImageItem = nullptr;

			if ( mfcImageIndex != -1 )
				pImageItem = new CToolbarImageItem( pImages, mfcImageIndex, cmdId, pCmdName, pCmdLiteral );			// use CMFCToolBarImages drawing
			else
				if ( const CIcon* pIcon = m_pImageStore->RetrieveIcon( CIconId( cmdId, SmallIcon ) ) )
					pImageItem = new CIconImageItem( pIcon, itBtnInfo->m_imagePos, cmdId, pCmdName, pCmdLiteral );	// use CIcon drawing

			if ( pImageItem != nullptr )
			{
				pImageItem->m_indexText = str::Format( _T("B-%d"), itBtnInfo->m_imagePos );
				if ( mfcImageIndex != -1 )
					pImageItem->m_indexText += str::Format( _T(" [M-%d]"), mfcImageIndex );		// append CMFCToolBarImages image index

				m_imageItems.push_back( pImageItem );
				pToolbarGroup->second.push_back( pImageItem );
			}
		}

		totalImageCount += static_cast<UINT>( btnInfos.size() );
	}

	m_imageCountText = str::Format( _T("Total: %d buttons grouped in %d toolbars"), m_imageItems.size(), toolbarDescrs.size() );
}

CStoreToolbarImagesPage::~CStoreToolbarImagesPage()
{
}

void CStoreToolbarImagesPage::AddListItems( void ) override
{
	CScopedLockRedraw freeze( &m_imageListCtrl );

	m_imageListCtrl.RemoveAllGroups();
	m_imageListCtrl.EnableGroupView( !m_toolbarGroups.empty() );

	for ( UINT groupId = 0, itemPos = 0; groupId != m_toolbarGroups.size(); ++groupId )
	{
		const TToolbarGroupPair* pToolbarGroup = &m_toolbarGroups[ groupId ];

		m_imageListCtrl.InsertGroupHeader( groupId, groupId, pToolbarGroup->first, LVGS_NORMAL | LVGS_COLLAPSIBLE );

		for ( std::vector<CBaseImageItem*>::const_iterator itImageItem = pToolbarGroup->second.begin(); itImageItem != pToolbarGroup->second.end(); ++itImageItem, ++itemPos )
		{
			AddListItem( itemPos, *itImageItem );
			VERIFY( m_imageListCtrl.SetItemGroupId( itemPos, groupId ) );
		}
	}
}


// CIconImagesPage implementation

IMPLEMENT_DYNCREATE( CIconImagesPage, CBaseStoreImagesPage )

CIconImagesPage::CIconImagesPage( ui::IImageStore* pImageStore /*= nullptr*/ )
	: CBaseStoreImagesPage( reg::s_pageTitle[ CToolbarImagesDialog::IconImagesPage ], pImageStore )
{
	std::vector<ui::CIconKey> iconKeys;
	m_pImageStore->QueryIconKeys( iconKeys, AnyIconSize );

	const mfc::CImageCommandLookup* pImageCmds = mfc::CImageCommandLookup::Instance();
	std::tstring customNameText;

	m_imageItems.reserve( iconKeys.size() );
	for ( UINT index = 0; index != iconKeys.size(); ++index )
	{
		const ui::CIconKey& iconKey = iconKeys[ index ];
		UINT cmdId = iconKey.m_iconResId;

		if ( const CIcon* pIcon = m_pImageStore->RetrieveIcon( CIconId( cmdId, iconKey.m_stdSize ) ) )
		{
			const std::tstring* pCmdName = pImageCmds->FindCommandName( cmdId );
			const std::tstring* pCmdLiteral = pImageCmds->FindCommandLiteral( cmdId );

			if ( nullptr == pCmdName || pCmdName->empty() )
				if ( const ui::CCmdTag* pIconTag = ui::CCmdTagStore::Instance().RetrieveTag( cmdId ) )
				{
					customNameText = pIconTag->GetTooltipText( true );
					if ( !customNameText.empty() )
						pCmdName = &customNameText;
				}

			CBaseImageItem* pImageItem = new CIconImageItem( pIcon, index, cmdId, pCmdName, pCmdLiteral );
			pImageItem->m_indexText = str::Format( _T("I-%d"), index );

			m_imageItems.push_back( pImageItem );
		}
	}

	m_imageCountText = str::Format( _T("Total: %d store icons"), m_imageItems.size() );
}

CIconImagesPage::~CIconImagesPage()
{
}
