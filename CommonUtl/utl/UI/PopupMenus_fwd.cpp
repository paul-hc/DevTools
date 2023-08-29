
#include "pch.h"
#include "PopupMenus_fwd.h"
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


	void* PopupMenu_FindButtonItemData( const CMFCPopupMenu* pPopupMenu, UINT btnId )
	{
		CMFCToolBarButton* pFoundButton = FindBarButton( pPopupMenu, btnId );

		if ( nullptr == pFoundButton )
		{
			ASSERT( false );
			return 0;
		}

		return ToolBarButton_GetItemData( pFoundButton );
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

	CMFCToolBarButton* FindBarButton( const CMFCPopupMenu* pPopupMenu, UINT btnId )
	{	// buttons reside in the embedded bar
		return pPopupMenu != nullptr ? ToolBar_FindButton( const_cast<CMFCPopupMenu*>( pPopupMenu )->GetMenuBar(), btnId ) : nullptr;
	}


	CMFCColorBar* GetColorMenuBar( const CMFCPopupMenu* pColorPopupMenu )
	{
		ASSERT( is_a<CMFCColorPopupMenu>( pColorPopupMenu ) );
		return checked_static_cast<CMFCColorBar*>( const_cast<CMFCPopupMenu*>( pColorPopupMenu )->GetMenuBar() );
	}
}
