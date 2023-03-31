
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

	void UpdateMenuUI( CWnd* pWnd, CMenu* pPopupMenu, bool autoMenuEnable /*= true*/ )
	{	// verbatim from CFrameWnd::OnInitMenuPopup
		CCmdUI itemState;

		itemState.m_pMenu = pPopupMenu;
		ASSERT( itemState.m_pOther == nullptr );
		ASSERT( itemState.m_pParentMenu == nullptr );

		// determine if menu is popup in top-level menu and set m_pOther to
		//  it if so (m_pParentMenu == nullptr indicates that it is secondary popup)
		HMENU hParentMenu;
		if ( AfxGetThreadState()->m_hTrackingMenu == pPopupMenu->m_hMenu )
			itemState.m_pParentMenu = pPopupMenu;					// parent == child for tracking popup
		else if ( ( hParentMenu = ::GetMenu( pWnd->m_hWnd ) ) != nullptr )
		{
			CWnd* pParent = pWnd->GetTopLevelParent();		// child windows don't have menus - need to go to the top

			if ( pParent != nullptr &&
				( hParentMenu = ::GetMenu( pParent->m_hWnd ) ) != nullptr )
			{
				int nIndexMax = ::GetMenuItemCount( hParentMenu );
				for ( int nIndex = 0; nIndex < nIndexMax; nIndex++ )
					if ( ::GetSubMenu( hParentMenu, nIndex ) == pPopupMenu->m_hMenu )	// popup found?
					{
						itemState.m_pParentMenu = CMenu::FromHandle( hParentMenu );			// m_pParentMenu is the containing menu
						break;
					}
			}
		}

		itemState.m_nIndexMax = pPopupMenu->GetMenuItemCount();
		for ( itemState.m_nIndex = 0; itemState.m_nIndex < itemState.m_nIndexMax; ++itemState.m_nIndex )
		{
			itemState.m_nID = pPopupMenu->GetMenuItemID( itemState.m_nIndex );
			if ( itemState.m_nID == 0 )
				continue;								// menu separator or invalid cmd - ignore it

			ASSERT_NULL( itemState.m_pOther );
			ASSERT_PTR( itemState.m_pMenu );

			if ( itemState.m_nID == (UINT)-1 )
			{
				// possibly a popup menu, route to first item of that popup
				itemState.m_pSubMenu = pPopupMenu->GetSubMenu( itemState.m_nIndex );
				if ( itemState.m_pSubMenu == nullptr ||
					( itemState.m_nID = itemState.m_pSubMenu->GetMenuItemID( 0 ) ) == 0 || itemState.m_nID == (UINT)-1 )
					continue;	   // first item of popup can't be routed to

				itemState.DoUpdate( pWnd, FALSE );		// popups are never auto disabled
			}
			else
			{
				// normal menu item
				// auto enable/disable according to autoMenuEnable
				//	set and command is _not_ a system command.
				itemState.m_pSubMenu = nullptr;
				itemState.DoUpdate( pWnd, !autoMenuEnable && itemState.m_nID < 0xF000 );
			}

			// adjust for menu deletions and additions
			UINT itemCount = pPopupMenu->GetMenuItemCount();
			if ( itemCount < itemState.m_nIndexMax )
			{
				itemState.m_nIndex -= ( itemState.m_nIndexMax - itemCount );
				while ( itemState.m_nIndex < itemCount && pPopupMenu->GetMenuItemID( itemState.m_nIndex ) == itemState.m_nID )
					++itemState.m_nIndex;
			}
			itemState.m_nIndexMax = itemCount;
		}
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
