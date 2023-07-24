
#include "pch.h"
#include "PopupMenus_fwd.h"
#include <afxpopupmenu.h>
#include <afxcolorpopupmenu.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace nosy
{
	struct CToolBarButton_ : public CMFCToolBarButton
	{
		// public access
		void* GetItemData( void ) const { return reinterpret_cast<void*>( m_dwdItemData ); }
	};

	struct CColorBar_ : public CMFCColorBar
	{
		// public access
		using CMFCColorBar::InitColors;
		using CMFCColorBar::m_ColorNames;		// CMap<COLORREF,COLORREF,CString, LPCTSTR>
	};
}


namespace mfc
{
	int ColorBar_InitColors( ui::TMFCColorArray& colors, CPalette* pPalette /*= nullptr*/ )
	{
		return nosy::CColorBar_::InitColors( pPalette, colors );
	}

	bool ColorBar_FindColorName( COLORREF realColor, OUT OPTIONAL std::tstring* pColorName /*= nullptr*/ )
	{
		CString colorName;

		if ( !nosy::CColorBar_::m_ColorNames.Lookup( realColor, colorName ) )
			return false;

		if ( pColorName != nullptr )
			*pColorName = colorName.GetString();

		return true;
	}

	void ColorBar_RegisterColorName( COLORREF realColor, const std::tstring& colorName )
	{
		CMFCColorBar::SetColorName( realColor, colorName.c_str() );
	}


	void* GetItemData( const CMFCToolBarButton* pButton )
	{
		return mfc::nosy_cast<nosy::CToolBarButton_>( pButton )->GetItemData();
	}

	void* GetButtonItemData( const CMFCPopupMenu* pPopupMenu, UINT btnId )
	{
		CMFCToolBarButton* pFoundButton = FindBarButton( pPopupMenu, btnId );

		if ( nullptr == pFoundButton )
		{
			ASSERT( false );
			return 0;
		}

		return GetItemData( pFoundButton );
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
