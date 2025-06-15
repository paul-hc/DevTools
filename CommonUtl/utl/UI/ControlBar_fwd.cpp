
#include "pch.h"
#include "ControlBar_fwd.h"
#include "WndUtils.h"
#include "utl/Algorithms_fwd.h"
#include "utl/ScopedValue.h"
#include <afxbasepane.h>
#include <afxstatusbar.h>
#include <afxdockingmanager.h>
#include <afxdropdowntoolbar.h>		// for is_a()
#include <afxtoolbareditboxbutton.h>
#include <afxtoolbarcomboboxbutton.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace nosy
{
	struct CToolBarImages_ : public CMFCToolBarImages
	{
		// public access
		using CMFCToolBarImages::m_nBitsPerPixel;
		using CMFCToolBarImages::m_sizeImageDest;
		using CMFCToolBarImages::m_bStretch;

		// static public access
		using CMFCToolBarImages::m_nDisabledImageAlpha;
		using CMFCToolBarImages::m_nFadedImageAlpha;
	};


	struct CBasePane_ : public CBasePane
	{
		// public access
		void SetIsDialogControl( bool isDlgControl = true ) { m_bIsDlgControl = isDlgControl; }
	};


	struct CToolBar_ : public CMFCToolBar
	{
		// public access
		using CMFCToolBar::m_DefaultImages;
		using CMFCToolBar::m_bMasked;
		using CMFCToolBar::AllowShowOnList;

		CToolTipCtrl* GetToolTip( void ) const { return m_pToolTip; }
	};


	struct CStatusBar_ : public CMFCStatusBar
	{
		// public access
		using CMFCStatusBar::_GetPanePtr;
		using CMFCStatusBar::GetCurrentFont;
	};


	struct CToolBarButton_ : public CMFCToolBarButton
	{
		// public access
		void* GetItemData( void ) const { return reinterpret_cast<void*>( m_dwdItemData ); }
		void SetItemData( const void* pItemData ) { utl::StoreValueAs( m_dwdItemData, pItemData ); }

		CRect GetImageBoundsRect( void ) const
		{
			CRect imageBoundsRect = m_rect;

			imageBoundsRect.left += CMFCVisualManager::GetInstance()->GetMenuImageMargin();
			imageBoundsRect.right = imageBoundsRect.left + CMFCToolBar::GetMenuImageSize().cx + CMFCVisualManager::GetInstance()->GetMenuImageMargin();
			return imageBoundsRect;
		}

		CRect GetImageRect( bool bounds = true ) const
		{
			CRect imageRect = m_rect;

			if ( bounds )
			{
				int margin = CMFCVisualManager::GetInstance()->GetMenuImageMargin();
				imageRect.DeflateRect( margin, margin );
			}
			return imageRect;
		}

		void RedrawImage( void )
		{
			if ( m_pWndParent != nullptr )
			{
				CRect imageRect = GetImageRect();

				m_pWndParent->InvalidateRect( &imageRect );
				m_pWndParent->UpdateWindow();
			}
		}
	};
}


namespace mfc
{
	const TCHAR CColorLabels::s_autoLabel[] = _T("Automatic");
	const TCHAR CColorLabels::s_moreLabel[] = _T("More Colors...");


	// CCommandManager utils

	int FindButtonImageIndex( UINT btnId, bool userImage /*= false*/ )
	{
		return GetCmdMgr()->GetCmdImage( btnId, userImage );
	}

	bool RegisterCmdImageAlias( UINT aliasCmdId, UINT imageCmdId )
	{
		ASSERT_PTR( GetCmdMgr() );
		// register command image alias for MFC control-bars
		int iconImagePos = GetCmdMgr()->GetCmdImage( imageCmdId );

		if ( iconImagePos != -1 )
		{
			GetCmdMgr()->SetCmdImage( aliasCmdId, iconImagePos, false );
			return true;			// image registered for alias command
		}
		else
		{
			TRACE( "\n ~ Warning: Could not find image for command ID %d (0x%04X).  Must load it with CMFCToolBar::AddToolBarForImageCollection( toolBarId ).\n", imageCmdId, imageCmdId );
			//ASSERT( false );
		}

		return false;
	}

	void RegisterCmdImageAliases( const ui::CCmdAlias cmdAliases[], size_t count )
	{
		for ( size_t i = 0; i != count; ++i )
			RegisterCmdImageAlias( cmdAliases[i].m_cmdId, cmdAliases[i].m_imageCmdId );
	}


	// CScopedCmdImageAliases implementation

	CScopedCmdImageAliases::CScopedCmdImageAliases( UINT aliasCmdId, UINT imageCmdId )
	{
		m_oldCmdImages.push_back( TCmdImagePair( aliasCmdId, GetCmdMgr()->GetCmdImage( imageCmdId ) ) );		// store original image index
		mfc::RegisterCmdImageAlias( aliasCmdId, imageCmdId );
	}

	CScopedCmdImageAliases::CScopedCmdImageAliases( const ui::CCmdAlias cmdAliases[], size_t count )
	{
		m_oldCmdImages.reserve( count );

		for ( size_t i = 0; i != count; ++i )
		{
			m_oldCmdImages.push_back( TCmdImagePair( cmdAliases[i].m_cmdId, GetCmdMgr()->GetCmdImage( cmdAliases[i].m_cmdId ) ) );	// store original image index
			RegisterCmdImageAlias( cmdAliases[i].m_cmdId, cmdAliases[i].m_imageCmdId );
		}
	}

	CScopedCmdImageAliases::~CScopedCmdImageAliases()
	{
		ASSERT_PTR( GetCmdMgr() );

		for ( std::vector<TCmdImagePair>::const_iterator itPair = m_oldCmdImages.begin(); itPair != m_oldCmdImages.end(); ++itPair )
			if ( itPair->second != -1 )
				GetCmdMgr()->SetCmdImage( itPair->first, itPair->second, false );
			else
				GetCmdMgr()->ClearCmdImage( itPair->first );
	}


	bool AssignTooltipText( OUT TOOLINFO* pToolInfo, const std::tstring& text )
	{
		ASSERT_PTR( pToolInfo );
		ASSERT_NULL( pToolInfo->lpszText );

		if ( text.empty() )
			return false;

		pToolInfo->lpszText = (TCHAR*)::calloc( text.length() + 1, sizeof( TCHAR ) );

		if ( nullptr == pToolInfo->lpszText )
			return false;

		_tcscpy( pToolInfo->lpszText, text.c_str() );

		if ( text.find( '\n' ) != std::tstring::npos )		// multi-line text?
			if ( const CMFCPopupMenuBar* pMenuBar = dynamic_cast<const CMFCPopupMenuBar*>( CWnd::FromHandlePermanent( pToolInfo->hwnd ) ) )
				if ( CToolTipCtrl* pToolTip = mfc::ToolBar_GetToolTip( pMenuBar ) )
				{
					// Win32 requirement for multi-line tooltips: we must send TTM_SETMAXTIPWIDTH to the tooltip
					if ( -1 == pToolTip->GetMaxTipWidth() )	// not initialized?
						pToolTip->SetMaxTipWidth( ui::FindMonitorRect( pToolTip->GetSafeHwnd(), ui::Workspace ).Width() );		// the entire desktop width

					pToolTip->SetDelayTime( TTDT_AUTOPOP, 30 * 1000 );		// display for 1/2 minute (16-bit limit: it doesn't work beyond 32768 miliseconds)
				}

		return true;
	}
}


namespace mfc
{
	// CMFCToolBarImages protected access:

	int ToolBarImages_GetBitsPerPixel( CMFCToolBarImages* pImages /*= CMFCToolBar::GetImages()*/ )
	{
		ASSERT_PTR( pImages );

	#if _MFC_VER <= 0x0900		// MFC version 9.00
		return mfc::nosy_cast<nosy::CToolBarImages_>( pImages )->m_nBitsPerPixel;		// GetBitsPerPixel() missing in older MFC
	#else
		return pImages->GetBitsPerPixel();
	#endif
	}

	BYTE& ToolBarImages_RefDisabledImageAlpha( void )
	{
		return mfc::nosy_cast<nosy::CToolBarImages_>( CMFCToolBar::GetImages() )->m_nDisabledImageAlpha;
	}

	BYTE& ToolBarImages_RefFadedImageAlpha( void )
	{
		return mfc::nosy_cast<nosy::CToolBarImages_>( CMFCToolBar::GetImages() )->m_nFadedImageAlpha;
	}

	bool ToolBarImages_DrawStretch( CMFCToolBarImages* pImages, CDC* pDC, const CRect& destRect, int imageIndex,
									bool hilite /*= false*/, bool disabled /*= false*/, bool indeterminate /*= false*/, bool shadow /*= false*/, bool inactive /*= false*/,
									BYTE alphaSrc /*= 255*/ )
	{
		nosy::CToolBarImages_* pNosyImages = mfc::nosy_cast<nosy::CToolBarImages_>( pImages );
		CScopedValue<CSize> scDestImageSize( &pNosyImages->m_sizeImageDest, destRect.Size() );		// temp destination size glyph
		CScopedValue<BOOL> scStretch( &pNosyImages->m_bStretch, TRUE );

		return pImages->Draw( pDC, destRect.left, destRect.top, imageIndex, hilite, disabled, indeterminate, shadow, inactive, alphaSrc ) != FALSE;
	}


	// CBasePane access

	void BasePane_SetIsDialogControl( OUT CBasePane* pBasePane, bool isDlgControl /*= true*/ )
	{
		mfc::nosy_cast<nosy::CBasePane_>( pBasePane )->SetIsDialogControl( isDlgControl );
	}


	// CMFCToolBar access

	const CMap<UINT, UINT, int, int>& ToolBar_GetDefaultImages( void )
	{
		return nosy::CToolBar_::m_DefaultImages;
	}

	CToolTipCtrl* ToolBar_GetToolTip( const CMFCToolBar* pToolBar )
	{
		return mfc::nosy_cast<nosy::CToolBar_>( pToolBar )->GetToolTip();
	}

	CMFCToolBarButton* ToolBar_FindButton( const CMFCToolBar* pToolBar, UINT btnId )
	{
		ASSERT_PTR( pToolBar );
		int btnIndex = pToolBar->CommandToIndex( btnId );

		return btnIndex != -1 ? pToolBar->GetButton( btnIndex ) : nullptr;
	}

	void ToolBar_SetBtnText( OUT CMFCToolBar* pToolBar, UINT btnId, const std::tstring& text, bool showText /*= true*/, bool showImage /*= true*/ )
	{
		ASSERT_PTR( pToolBar );
		int btnIndex = pToolBar->CommandToIndex( btnId );

		ENSURE( btnIndex != -1 );
		pToolBar->SetToolBarBtnText( btnIndex, text.c_str(), showText, showImage );
	}

	bool ToolBar_RestoreOriginalState( OUT CMFCToolBar* pToolBar )
	{
	#if _MFC_VER > 0x0900		// newer MFC version?
		return pToolBar->RestoreOriginalState() != FALSE;
	#else
		return pToolBar->RestoreOriginalstate() != FALSE;		// workaround typo in older MFC
	#endif
	}

	CMFCToolBarButton* ToolBar_ButtonHitTest( const CMFCToolBar* pToolBar, const CPoint& clientPos, OUT int* pBtnIndex /*= nullptr*/ )
	{
		int btnIndex = const_cast<CMFCToolBar*>( pToolBar )->HitTest( clientPos );

		utl::AssignPtr( pBtnIndex, btnIndex );
		if ( -1 == btnIndex )
			return nullptr;

		return pToolBar->GetButton( btnIndex );
	}


	// CMFCStatusBar access

	CMFCStatusBarPaneInfo* StatusBar_GetPaneInfo( const CMFCStatusBar* pStatusBar, int index )
	{
		return mfc::nosy_cast<nosy::CStatusBar_>( pStatusBar )->_GetPanePtr( index );
	}

	int StatusBar_CalcPaneTextWidth( const CMFCStatusBar* pStatusBar, const CMFCStatusBarPaneInfo* pPaneInfo )
	{
		ASSERT( pStatusBar != nullptr && pPaneInfo != nullptr );

		CClientDC screenDC( nullptr );
		HGDIOBJ hOldFont = screenDC.SelectObject( mfc::nosy_cast<nosy::CStatusBar_>( pStatusBar )->GetCurrentFont() );

		int textWidth = screenDC.GetTextExtent( pPaneInfo->lpszText, (int)str::GetLength( pPaneInfo->lpszText ) ).cx;

		screenDC.SelectObject( hOldFont );
		return textWidth;
	}

	int StatusBar_ResizePaneToFitText( OUT CMFCStatusBar* pStatusBar, int index )
	{
		CMFCStatusBarPaneInfo* pPaneInfo = StatusBar_GetPaneInfo( pStatusBar, index );

		pPaneInfo->cxText = mfc::StatusBar_CalcPaneTextWidth( pStatusBar, pPaneInfo );
		pStatusBar->SetPaneWidth( index, pPaneInfo->cxText );

		return pPaneInfo->cxText;
	}
}


namespace mfc
{
	// CMFCToolBarButton access:
	bool ToolBarButton_SetStyleFlag( CMFCToolBarButton* pButton, UINT styleFlag, bool on /*= true*/ )
	{
		ASSERT_PTR( pButton );

		UINT newStyle = pButton->m_nStyle;
		SetFlag( newStyle, styleFlag, on );

		if ( newStyle == pButton->m_nStyle )
			return false;

		pButton->SetStyle( newStyle );
		return true;
	}

	void* ToolBarButton_GetItemData( const CMFCToolBarButton* pButton )
	{
		return mfc::nosy_cast<nosy::CToolBarButton_>( pButton )->GetItemData();
	}

	void ToolBarButton_SetItemData( CMFCToolBarButton* pButton, const void* pItemData )
	{
		mfc::nosy_cast<nosy::CToolBarButton_>( pButton )->SetItemData( pItemData );
	}


	void ToolBarButton_SetImageById( CMFCToolBarButton* pButton, UINT btnId, bool userImage /*= false*/ )
	{
		ASSERT_PTR( pButton );
		pButton->SetImage( FindButtonImageIndex( btnId, userImage ) );
	}

	CRect ToolBarButton_GetImageRect( const CMFCToolBarButton* pButton, bool bounds /*= true*/ )
	{
		return mfc::nosy_cast<nosy::CToolBarButton_>( pButton )->GetImageRect( bounds );
	}


	// CMFCToolBarButton utils:

	bool ToolBarButton_EditSelectAll( CMFCToolBarButton* pButton )
	{	// select all text in the edit field
		ASSERT_PTR( pButton );
		CEdit* pEdit = nullptr;

		if ( CMFCToolBarComboBoxButton* pComboBtn = dynamic_cast<CMFCToolBarComboBoxButton*>( pButton ) )
			pEdit = pComboBtn->GetEditCtrl();
		else if ( CMFCToolBarEditBoxButton* pEditBtn = dynamic_cast<CMFCToolBarEditBoxButton*>( pButton ) )
			pEdit = pEditBtn->GetEditBox();

		if ( nullptr == pEdit->GetSafeHwnd() )
			return false;

		pEdit->SetSel( 0, -1 );
		return true;
	}

	void ToolBarButton_Redraw( CMFCToolBarButton* pButton )
	{
		ASSERT_PTR( pButton );

		if ( pButton->GetParentWnd()->GetSafeHwnd() != nullptr )
		{
			CRect rect = pButton->GetInvalidateRect();

			rect.InflateRect( 2, 2 );
			pButton->GetParentWnd()->RedrawWindow( &rect, nullptr, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE | RDW_FRAME );
		}

		if ( HWND hCtrl = pButton->GetHwnd() )
			::RedrawWindow( hCtrl, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE | RDW_FRAME );
	}

	void ToolBarButton_RedrawImage( CMFCToolBarButton* pButton )
	{
		return mfc::nosy_cast<nosy::CToolBarButton_>( pButton )->RedrawImage();
	}
}


namespace mfc
{
	int ColorBar_InitColors( mfc::TColorArray& colors, CPalette* pPalette /*= nullptr*/ )
	{
		return nosy::CColorBar_::InitColors( pPalette, colors );
	}
}


namespace mfc
{
	void QueryAllCustomizableControlBars( OUT std::vector<CMFCToolBar*>& rToolbars, CWnd* pFrameWnd /*= AfxGetMainWnd()*/ )
	{
		const CObList& allToolbars = CMFCToolBar::GetAllToolbars();

		for ( POSITION pos = allToolbars.GetHeadPosition(); pos != nullptr; )
		{
			nosy::CToolBar_* pToolBar = mfc::nosy_cast<nosy::CToolBar_>( (CMFCToolBar*)allToolbars.GetNext( pos ) );
			ASSERT_PTR( pToolBar );

			if ( CWnd::FromHandlePermanent( pToolBar->m_hWnd ) != nullptr )
				if ( !is_a<CMFCDropDownToolBar>( pToolBar ) )										// skip dropdown toolbars!
					if ( ( nullptr == pFrameWnd || pFrameWnd == pToolBar->GetTopLevelFrame() ) &&	// check if toolbar belongs to the frame window filter?
						 pToolBar->AllowShowOnList() && !pToolBar->m_bMasked )
					{
						rToolbars.push_back( pToolBar );
					}
		}
	}

	void ResetAllControlBars( CWnd* pFrameWnd /*= AfxGetMainWnd()*/ )
	{
		std::vector<CMFCToolBar*> toolbars;

		mfc::QueryAllCustomizableControlBars( toolbars, pFrameWnd );

		for ( std::vector<CMFCToolBar*>::const_iterator itToolbar = toolbars.begin(); itToolbar != toolbars.end(); ++itToolbar )
			if ( (*itToolbar)->CanBeRestored() )
				mfc::ToolBar_RestoreOriginalState( *itToolbar );

		// TODO: check whether we need to re-assign the command aliases after this!
	}


	// FrameWnd utils:

	void DockPanesOnRow( CDockingManager* pFrameDocManager, size_t barCount, CMFCToolBar* pFirstToolBar, ... )
	{
		REQUIRE( barCount != 0 );
		ASSERT_PTR( pFirstToolBar->GetSafeHwnd() );

		std::vector<CMFCToolBar*> toolbars;
		toolbars.push_back( pFirstToolBar );

		{	// push subsequent toolbars
			va_list argList;

			va_start( argList, pFirstToolBar );
			utl::QueryArgumentList( toolbars, argList, utl::npos == barCount ? barCount : ( barCount - 1 ) );		// pFirstToolBar already pushed
			va_end( argList );
		}

		// dock toolbars in reverse order: DockPane() last, and DockPaneLeftOf() subsequently
		size_t i = toolbars.size();
		CMFCToolBar* pRightToolbar = toolbars[ --i ];

		pFrameDocManager->DockPane( pRightToolbar );		// dock last toolbar on a new row

		for ( ; i-- != 0; pRightToolbar = toolbars[i] )
			pFrameDocManager->DockPaneLeftOf( toolbars[i] /*LEFT*/, pRightToolbar );
	}
}


namespace mfc
{
	// CFixedToolBar implementation

	CFixedToolBar::CFixedToolBar( void )
	{
		m_bDisableCustomize = true;
		SetPermament();		// m_bPermament = true;
	}

	CFixedToolBar::~CFixedToolBar()
	{
	}

	BOOL CFixedToolBar::OnUserToolTip( CMFCToolBarButton* pButton, CString& rTipText ) const overrides(CMFCToolBar)
	{
		if ( const ui::ICustomCmdInfo* pCmdInfo = dynamic_cast<const ui::ICustomCmdInfo*>( GetOwner() ) )
		{	// owner frame implements ui::ICustomCmdInfo
			std::tstring tipText;

			pCmdInfo->QueryTooltipText( tipText, pButton->m_nID, nullptr );
			if ( !tipText.empty() )
			{
				rTipText = tipText.c_str();
				return TRUE;
			}
		}

		return __super::OnUserToolTip( pButton, rTipText );
	}


	// message handlers

	BEGIN_MESSAGE_MAP( CFixedToolBar, CMFCToolBar )
		ON_WM_CREATE()
	END_MESSAGE_MAP()

	int CFixedToolBar::OnCreate( CREATESTRUCT* pCS )
	{
		if ( -1 == __super::OnCreate( pCS ) )
			return -1;

		CObList& rAllToolbars = const_cast<CObList&>( CMFCToolBar::GetAllToolbars() );

		ASSERT( rAllToolbars.GetTail() == this );
		rAllToolbars.RemoveTail();		// prevent buttons updating across different album bars (!)
		return 0;
	}
}
