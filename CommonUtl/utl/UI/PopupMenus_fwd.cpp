
#include "pch.h"
#include "PopupMenus_fwd.h"
#include "Color.h"
#include "ColorRepository.h"
#include "WndUtils.h"
#include <afxpopupmenu.h>
#include <afxcolorpopupmenu.h>
#include <afxbutton.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace ui
{
	bool IColorEditorHost::EditColorDialog( void )
	{
		CWnd* pHostWnd = GetHostWindow();
		ui::TDisplayColor color = ui::EvalColor( GetActualColor() );

		if ( !ui::EditColor( &color, pHostWnd, true ) )
			return false;

		SetColor( color, true );
		return true;
	}

	bool IColorEditorHost::SwitchSelColorTable( const CColorTable* pSelColorTable )
	{	// when using a Shades_Colors or UserCustom_Colors table, color can be picked from any table, but the selected table is retained (not switched to the picked table)
		if ( UseUserColors() || ( pSelColorTable != nullptr && ui::Shades_Colors == pSelColorTable->GetTableType() ) )
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


	struct CBasePane_ : public CBasePane
	{
		// public access
		void SetIsDialogControl( bool isDlgControl = true ) { m_bIsDlgControl = isDlgControl; }
	};


	struct CPopupMenu_ : public CMFCPopupMenu
	{
		// public access
		using CMFCPopupMenu::m_bTrackMode;
	};


	struct CToolBar_ : public CMFCToolBar
	{
		// public access
		CToolTipCtrl* GetToolTip( void ) const { return m_pToolTip; }
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


	void BasePane_SetIsDialogControl( CBasePane* pBasePane, bool isDlgControl /*= true*/ )
	{
		mfc::nosy_cast<nosy::CBasePane_>( pBasePane )->SetIsDialogControl( isDlgControl );
	}


	CToolTipCtrl* ToolBar_GetToolTip( const CMFCToolBar* pToolBar )
	{
		return mfc::nosy_cast<nosy::CToolBar_>( pToolBar )->GetToolTip();
	}

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
}
