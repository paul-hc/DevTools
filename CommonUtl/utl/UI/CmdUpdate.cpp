
#include "pch.h"
#include "CmdUpdate.h"
#include "MenuUtilities.h"
#include "WndUtils.h"
#include "VersionInfo.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace ui
{
	std::tstring GetCmdText( const CCmdUI* pCmdUI )
	{
		ASSERT_PTR( pCmdUI );

		if ( pCmdUI->m_pMenu != nullptr )
			return ui::GetMenuItemText( *pCmdUI->m_pMenu, pCmdUI->m_nID, MF_BYCOMMAND );
		else if ( pCmdUI->m_pOther != nullptr )
			return ui::GetWindowText( pCmdUI->m_pOther );

		return std::tstring();
	}


	void SetRadio( CCmdUI* pCmdUI, BOOL checked )
	{
		ASSERT_PTR( pCmdUI );
		if ( nullptr == pCmdUI->m_pMenu )
		{
			pCmdUI->SetRadio( checked );		// normal processing for toolbar buttons, etc
			return;
		}

		// CCmdUI::SetRadio() uses an ugly radio checkmark;
		// we put the standard nice radio checkmark using CheckMenuRadioItem()
		if ( !checked )
			pCmdUI->SetCheck( checked );
		else
		{
			if ( pCmdUI->m_pSubMenu != nullptr )
				return;							// don't change popup submenus indirectly

			UINT pos = pCmdUI->m_nIndex;
			pCmdUI->m_pMenu->CheckMenuRadioItem( pos, pos, pos, MF_BYPOSITION );		// place radio checkmark
		}
	}

	bool ExpandVersionInfoTags( CCmdUI* pCmdUI )
	{
		ASSERT_PTR( pCmdUI );
		if ( pCmdUI->m_pMenu != nullptr && nullptr == pCmdUI->m_pSubMenu )		// a menu but not sub-menu (don't change submenus indirectly, wait for their expansion)
		{
			static CVersionInfo s_versionInfo;
			std::tstring menuItemText = GetMenuItemText( *pCmdUI->m_pMenu, pCmdUI->m_nID );
			std::tstring newMenuItemText = s_versionInfo.ExpandValues( menuItemText.c_str() );

			if ( newMenuItemText != menuItemText )
			{
				pCmdUI->SetText( newMenuItemText.c_str() );
				return true;
			}
		}
		return false;
	}


	namespace impl
	{
		CMenu* FindParentMenu( const CWnd* pTargetWnd, const CMenu* pMenu )
		{	// locate the parent menu for frame windows (non-tracking) case
			ASSERT_PTR( pTargetWnd->GetSafeHwnd() );
			ASSERT_PTR( pMenu->GetSafeHmenu() );

			if ( HMENU hParentMenu = ::GetMenu( pTargetWnd->m_hWnd ) )
				if ( CWnd* pTopWnd = pTargetWnd->GetTopLevelParent() )				// child windows don't have menus - need to go to the top
					if ( ( hParentMenu = ::GetMenu( pTopWnd->m_hWnd ) ) != nullptr )
						for ( int index = 0, count = ::GetMenuItemCount( hParentMenu ); index != count; ++index )
							if ( ::GetSubMenu( hParentMenu, index ) == pMenu->m_hMenu )		// popup found?
								return CMenu::FromHandle( hParentMenu );			// m_pParentMenu is the containing menu

			return nullptr;
		}

		void UpdateMenuUI_Impl( CWnd* pTargetWnd, CMenu* pMenu, CMenu* pParentMenu, bool autoMenuEnable, RecursionDepth depth )
		{
			// Note: Deep recursion depth is useful only for tracking menus with TPM_NONOTIFY flag,
			// when WM_INITMENUPOPUP is not sent for pMenu or it's sub-menus.
			ASSERT_PTR( pTargetWnd->GetSafeHwnd() );

			CCmdUI itemState;

			itemState.m_pMenu = pMenu;
			itemState.m_pParentMenu = pParentMenu;
			ASSERT_NULL( itemState.m_pOther );

			itemState.m_nIndexMax = pMenu->GetMenuItemCount();
			for ( itemState.m_nIndex = 0; itemState.m_nIndex < itemState.m_nIndexMax; ++itemState.m_nIndex )
			{
				itemState.m_nID = pMenu->GetMenuItemID( itemState.m_nIndex );
				if ( 0 == itemState.m_nID )
					continue;								// menu separator or invalid cmd - ignore it

				ASSERT_NULL( itemState.m_pOther );
				ASSERT_PTR( itemState.m_pMenu );

				if ( UINT_MAX == itemState.m_nID )			// sub-menu?
				{	// sub-menu popup, route to first command in that popup
					itemState.m_pSubMenu = pMenu->GetSubMenu( itemState.m_nIndex );

					if ( itemState.m_pSubMenu != nullptr )
					{
						if ( ui::FindFirstMenuCommand( &itemState.m_nID, *itemState.m_pSubMenu ) != nullptr )	// use the ID of the first command of the sub-menu
							itemState.DoUpdate( pTargetWnd, FALSE );		// update the popup item (popups are never auto disabled)

						if ( Deep == depth )
							UpdateMenuUI_Impl( pTargetWnd, itemState.m_pSubMenu, pMenu, autoMenuEnable, depth );
					}
				}
				else
				{
					// normal menu item
					// auto enable/disable according to autoMenuEnable
					//	set and command is _not_ a system command.
					itemState.m_pSubMenu = nullptr;
					itemState.DoUpdate( pTargetWnd, !autoMenuEnable && itemState.m_nID < 0xF000 );
				}

				// adjust for menu deletions and additions
				UINT itemCount = pMenu->GetMenuItemCount();
				if ( itemCount < itemState.m_nIndexMax )
				{
					itemState.m_nIndex -= ( itemState.m_nIndexMax - itemCount );

					while ( itemState.m_nIndex < itemCount && pMenu->GetMenuItemID( itemState.m_nIndex ) == itemState.m_nID )
						++itemState.m_nIndex;
				}
				itemState.m_nIndexMax = itemCount;
			}
		}
	}

	void UpdateMenuUI( CWnd* pTargetWnd, CMenu* pMenu, bool autoMenuEnable /*= true*/, bool isTracking /*= false*/, RecursionDepth depth /*= Shallow*/ )
	{	// verbatim from CFrameWnd::OnInitMenuPopup.

		// determine if menu is popup in top-level menu and set m_pOther to
		//  it if so (m_pParentMenu == nullptr indicates that it is secondary popup)

		CMenu* pParentMenu = nullptr;

		if ( isTracking || AfxGetThreadState()->m_hTrackingMenu == pMenu->m_hMenu )
			pParentMenu = pMenu;					// parent == child for tracking popup
		else
			pParentMenu = impl::FindParentMenu( pTargetWnd, pMenu );		// locate the parent menu for frame windows menu update

		impl::UpdateMenuUI_Impl( pTargetWnd, pMenu, pParentMenu, autoMenuEnable, depth );
	}


	CCmdTarget* ResolveDlgTarget( CCmdTarget*& rpTarget, HWND hDlg )
	{
		REQUIRE( ::IsWindow( hDlg ) );
		if ( nullptr == rpTarget )
			rpTarget = CWnd::FromHandlePermanent( hDlg );

		ASSERT_PTR( rpTarget );
		return rpTarget;
	}

	CCmdTarget* ResolveCtrlTarget( CCmdTarget*& rpTarget, HWND hCtrl )
	{
		REQUIRE( ::IsWindow( hCtrl ) );
		if ( nullptr == rpTarget )
			rpTarget = CWnd::FromHandlePermanent( ::GetParent( hCtrl ) );

		ASSERT_PTR( rpTarget );
		return rpTarget;
	}


	void UpdateDlgControlsUI( HWND hDlg, CCmdTarget* pTarget /*= nullptr*/, bool disableIfNoHandler /*= false*/ )
	{
		ui::ResolveDlgTarget( pTarget, hDlg );

		for ( HWND hCtrl = ::GetTopWindow( hDlg ); hCtrl != nullptr; hCtrl = ::GetNextWindow( hCtrl, GW_HWNDNEXT ) )
			UpdateControlUI( hCtrl, pTarget, disableIfNoHandler );
	}

	void UpdateDlgControlsUI( HWND hDlg, const UINT ctrlIds[], size_t count, CCmdTarget* pTarget /*= nullptr*/, bool disableIfNoHandler /*= false*/ )
	{
		ui::ResolveDlgTarget( pTarget, hDlg );

		for ( size_t i = 0; i != count; ++i )
			if ( HWND hCtrl = ::GetDlgItem( hDlg, ctrlIds[ i ] ) )
				UpdateControlUI( hCtrl, pTarget, disableIfNoHandler );
	}

	bool UpdateControlUI( HWND hCtrl, CCmdTarget* pTarget /*= nullptr*/, bool disableIfNoHandler /*= false*/ )
	{	// inspired from CWnd::UpdateDialogControls() MFC implementation
		ui::ResolveCtrlTarget( pTarget, hCtrl );

		CCmdUI ctrlState;
		CWnd tempWnd;		// temporary window just for CmdUI update

		// send to buttons
		tempWnd.m_hWnd = hCtrl; // quick and dirty attach
		ctrlState.m_nID = static_cast<UINT>( ::GetDlgCtrlID( hCtrl ) );
		ctrlState.m_pOther = &tempWnd;

		// check for reflect handlers in the child window
		CWnd* pWnd = CWnd::FromHandlePermanent( hCtrl );
		if ( pWnd != nullptr )
		{
			// call it directly to disable any routing
			if ( pWnd->CWnd::OnCmdMsg( 0, MAKELONG( 0xFFFF, WM_COMMAND + WM_REFLECT_BASE ), &ctrlState, nullptr ) )
				return true;		// handled
		}

		if ( CWnd* pParentWnd = CWnd::FromHandlePermanent( hCtrl ) )	// check for handlers in the parent window
			if ( pParentWnd->CWnd::OnCmdMsg( ctrlState.m_nID, CN_UPDATE_COMMAND_UI, &ctrlState, nullptr ) )
				return true;		// handled

		// determine whether to disable when no handler exists
		bool disableCtrl = disableIfNoHandler;
		if ( disableCtrl )
			if ( !HasFlag( tempWnd.SendMessage( WM_GETDLGCODE ), DLGC_BUTTON ) )
				disableCtrl = false;		// non-button controls don't get automagically disabled
			else
				switch ( tempWnd.GetStyle() & BS_TYPEMASK )
				{	// only certain button controls get automagically disabled
					case BS_AUTOCHECKBOX:
					case BS_AUTO3STATE:
					case BS_GROUPBOX:
					case BS_AUTORADIOBUTTON:
						disableCtrl = false;
				}

		bool handled = ctrlState.DoUpdate( pTarget, disableCtrl ) != FALSE;		// check for handlers in the target (owner)

		tempWnd.m_hWnd = nullptr;		// quick and dirty detach
		return handled;
	}

	bool UpdateControlUI_utlOld( CWnd* pCtrl, CWnd* pTargetWnd /*= nullptr*/ )
	{
		if ( nullptr == pTargetWnd )
			pTargetWnd = pCtrl->GetParent();

		ASSERT_PTR( pCtrl->GetSafeHwnd() );
		ASSERT_PTR( pTargetWnd->GetSafeHwnd() );

		CCmdUI ctrlState;

		ctrlState.m_nID = pCtrl->GetDlgCtrlID();
		if ( -1 == ui::ToIntCmdId( ctrlState.m_nID ) )
			return false;			// avoid anonymous statics and group-boxes

		ctrlState.m_pOther = pCtrl;
		return ctrlState.DoUpdate( pTargetWnd, FALSE ) != FALSE;
	}
}


namespace ui
{
	void HandleInitMenuPopup( CWnd* pTargetWnd, CMenu* pPopupMenu, bool isUserMenu )
	{
		ASSERT_PTR( pTargetWnd->GetSafeHwnd() );

		::AfxCancelModes( pTargetWnd->GetSafeHwnd() );		// cancel any menus or combobox popups that could be in toolbars or dialog bars

		if ( isUserMenu )
			ui::UpdateMenuUI( pTargetWnd, pPopupMenu );
	}
}
