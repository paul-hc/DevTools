
#include "pch.h"
#include "PopupMenus_fwd.h"
#include "ControlBar_fwd.h"
#include "Color.h"
#include "ColorRepository.h"
#include "WndUtils.h"
#include "utl/ScopedValue.h"
#include <afxpopupmenu.h>
#include <afxcolorpopupmenu.h>
#include <afxbutton.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace ui
{
	// IColorHost implementation

	bool IColorHost::IsForeignColor( void ) const
	{
		COLORREF selColor = GetColor();
		const CColorTable* pSelColorTable = GetSelColorTable();

		return selColor != CLR_NONE && ( nullptr == pSelColorTable || !pSelColorTable->ContainsColor( selColor ) );
	}


	// IColorEditorHost implementation

	bool IColorEditorHost::EditColorDialog( void )
	{
		CWnd* pHostWnd = GetHostWindow();
		ui::TDisplayColor color = ui::EvalColor( GetActualColor() );

		if ( !ui::EditColor( &color, pHostWnd, !ui::IsKeyPressed( VK_CONTROL ) ) )
			return false;

		SetColor( color, true );
		return true;
	}

	bool IColorEditorHost::SwitchSelColorTable( const CColorTable* pSelColorTable )
	{	// when using a Shades_Colors or UserCustom_Colors table, color can be picked from any table, but the selected table is retained (not switched to the picked table)
		if ( pSelColorTable != nullptr && CScratchColorStore::Instance() == pSelColorTable->GetParentStore() )
			return false;						// prevent switching the 'read-only' table?

		SetSelColorTable( pSelColorTable );		// switch to the new table
		return true;
	}
}


namespace nosy
{
	struct CToolBarButton_ : public CMFCToolBarButton
	{
		// public access
		void* GetItemData( void ) const { return reinterpret_cast<void*>( m_dwdItemData ); }
		void SetItemData( const void* pItemData ) { m_dwdItemData = reinterpret_cast<DWORD_PTR>( pItemData ); }

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


	struct CPopupMenu_ : public CMFCPopupMenu
	{
		// public access
		using CMFCPopupMenu::m_bTrackMode;
	};


	struct CMFCButton_ : public CMFCButton
	{
		// public access
		using CMFCButton::m_bCaptured;
	};
}


namespace mfc
{
	const TCHAR CColorLabels::s_autoLabel[] = _T("Automatic");
	const TCHAR CColorLabels::s_moreLabel[] = _T("More Colors...");


	bool RegisterCmdImageAlias( UINT aliasCmdId, UINT imageCmdId )
	{
		ASSERT_PTR( afxCommandManager );
		// register command image alias for MFC control-bars
		int iconImagePos = afxCommandManager->GetCmdImage( imageCmdId );

		if ( iconImagePos != -1 )
		{
			afxCommandManager->SetCmdImage( aliasCmdId, iconImagePos, false );
			return true;			// image registered for alias command
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
		m_oldCmdImages.push_back( TCmdImagePair( aliasCmdId, afxCommandManager->GetCmdImage( imageCmdId ) ) );		// store original image index
		mfc::RegisterCmdImageAlias( aliasCmdId, imageCmdId );
	}

	CScopedCmdImageAliases::CScopedCmdImageAliases( const ui::CCmdAlias cmdAliases[], size_t count )
	{
		m_oldCmdImages.reserve( count );

		for ( size_t i = 0; i != count; ++i )
		{
			m_oldCmdImages.push_back( TCmdImagePair( cmdAliases[i].m_cmdId, afxCommandManager->GetCmdImage( cmdAliases[i].m_cmdId ) ) );	// store original image index
			RegisterCmdImageAlias( cmdAliases[i].m_cmdId, cmdAliases[i].m_imageCmdId );
		}
	}

	CScopedCmdImageAliases::~CScopedCmdImageAliases()
	{
		ASSERT_PTR( afxCommandManager );

		for ( std::vector<TCmdImagePair>::const_iterator itPair = m_oldCmdImages.begin(); itPair != m_oldCmdImages.end(); ++itPair )
			if ( itPair->second != -1 )
				afxCommandManager->SetCmdImage( itPair->first, itPair->second, false );
			else
				afxCommandManager->ClearCmdImage( itPair->first );
	}
}


namespace mfc
{
	CMFCToolBarButton* ToolBar_ButtonHitTest( const CMFCToolBar* pToolBar, const CPoint& clientPos, OUT int* pBtnIndex /*= nullptr*/ )
	{
		int btnIndex = const_cast<CMFCToolBar*>( pToolBar )->HitTest( clientPos );

		utl::AssignPtr( pBtnIndex, btnIndex );
		if ( -1 == btnIndex )
			return nullptr;

		return pToolBar->GetButton( btnIndex );
	}


	int ColorBar_InitColors( mfc::TColorArray& colors, CPalette* pPalette /*= nullptr*/ )
	{
		return nosy::CColorBar_::InitColors( pPalette, colors );
	}


	bool Button_SetStyleFlag( CMFCToolBarButton* pButton, UINT styleFlag, bool on /*= true*/ )
	{
		ASSERT_PTR( pButton );

		UINT newStyle = pButton->m_nStyle;
		SetFlag( newStyle, styleFlag, on );

		if ( newStyle == pButton->m_nStyle )
			return false;

		pButton->SetStyle( newStyle );
		return true;
	}

	void* Button_GetItemData( const CMFCToolBarButton* pButton )
	{
		return mfc::nosy_cast<nosy::CToolBarButton_>( pButton )->GetItemData();
	}

	void Button_SetItemData( CMFCToolBarButton* pButton, const void* pItemData )
	{
		mfc::nosy_cast<nosy::CToolBarButton_>( pButton )->SetItemData( pItemData );
	}

	void* Button_GetItemData( const CMFCPopupMenu* pPopupMenu, UINT btnId )
	{
		CMFCToolBarButton* pFoundButton = FindBarButton( pPopupMenu, btnId );

		if ( nullptr == pFoundButton )
		{
			ASSERT( false );
			return 0;
		}

		return Button_GetItemData( pFoundButton );
	}


	void Button_SetImageById( CMFCToolBarButton* pButton, UINT btnId, bool userImage /*= false*/ )
	{
		ASSERT_PTR( pButton );
		pButton->SetImage( Button_FindImageIndex( btnId, userImage ) );
	}

	int Button_FindImageIndex( UINT btnId, bool userImage /*= false*/ )
	{
		return afxCommandManager->GetCmdImage( btnId, userImage );
	}

	CRect Button_GetImageRect( const CMFCToolBarButton* pButton, bool bounds /*= true*/ )
	{
		return mfc::nosy_cast<nosy::CToolBarButton_>( pButton )->GetImageRect( bounds );
	}

	void Button_RedrawImage( CMFCToolBarButton* pButton )
	{
		return mfc::nosy_cast<nosy::CToolBarButton_>( pButton )->RedrawImage();
	}


	void MfcButton_SetCaptured( CMFCButton* pButton, bool captured )
	{
		mfc::nosy_cast<nosy::CMFCButton_>( pButton )->m_bCaptured = captured;
	}


	bool PopupMenu_InTrackMode( const CMFCPopupMenu* pPopupMenu )
	{
		return mfc::nosy_cast<nosy::CPopupMenu_>( pPopupMenu )->m_bTrackMode != FALSE;
	}

	void PopupMenu_SetTrackMode( CMFCPopupMenu* pPopupMenu, BOOL trackMode /*= true*/ )
	{
		mfc::nosy_cast<nosy::CPopupMenu_>( pPopupMenu )->m_bTrackMode = trackMode;
	}


	int PopupMenuBar_GetGutterWidth( CMFCPopupMenuBar* pPopupMenuBar )
	{
	#if _MFC_VER > 0x0900		// MFC version 9.00 or less
		CScopedValue<BOOL> scSideBar( &pPopupMenuBar->m_bDisableSideBarInXPMode, false );		// GetGutterWidth() returns 0 if m_bDisableSideBarInXPMode=true
		return const_cast<CMFCPopupMenuBar*>( pPopupMenuBar )->GetGutterWidth();
	#else
		pPopupMenuBar;
		return CMFCToolBar::GetMenuImageSize().cx + 2 * CMFCVisualManager::GetInstance()->GetMenuImageMargin() + 2;
	#endif
	}

	CMFCPopupMenu* GetSafePopupMenu( CMFCPopupMenu* pPopupMenu )
	{
		if ( pPopupMenu != nullptr && ::IsWindow( pPopupMenu->m_hWnd ) && CWnd::FromHandlePermanent( pPopupMenu->m_hWnd ) != nullptr )
			return pPopupMenu;

		return nullptr;
	}

	CMFCToolBarButton* FindToolBarButton( const CMFCToolBar* pToolBar, UINT btnId )
	{
		ASSERT_PTR( pToolBar );
		int btnIndex = pToolBar->CommandToIndex( btnId );

		return btnIndex != -1 ? pToolBar->GetButton( btnIndex ) : nullptr;
	}

	CMFCToolBarButton* FindBarButton( const CMFCPopupMenu* pPopupMenu, UINT btnId )
	{	// buttons reside in the embedded bar
		return pPopupMenu != nullptr ? FindToolBarButton( const_cast<CMFCPopupMenu*>( pPopupMenu )->GetMenuBar(), btnId ) : nullptr;
	}


	CMFCColorBar* GetColorMenuBar( const CMFCPopupMenu* pColorPopupMenu )
	{
		ASSERT( is_a<CMFCColorPopupMenu>( pColorPopupMenu ) );
		return checked_static_cast<CMFCColorBar*>( const_cast<CMFCPopupMenu*>( pColorPopupMenu )->GetMenuBar() );
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
