
#include "stdafx.h"
#include "CmdUpdate.h"
#include "MenuUtilities.h"
#include "Utilities.h"
#include "VersionInfo.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace ui
{
	std::tstring GetCmdText( const CCmdUI* pCmdUI )
	{
		ASSERT_PTR( pCmdUI );

		if ( pCmdUI->m_pMenu != NULL )
			return ui::GetMenuItemText( *pCmdUI->m_pMenu, pCmdUI->m_nID, MF_BYCOMMAND );
		else if ( pCmdUI->m_pOther != NULL )
			return ui::GetWindowText( pCmdUI->m_pOther );

		return std::tstring();
	}


	void SetRadio( CCmdUI* pCmdUI, BOOL checked )
	{
		ASSERT_PTR( pCmdUI );
		if ( NULL == pCmdUI->m_pMenu )
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
			if ( pCmdUI->m_pSubMenu != NULL )
				return;							// don't change popup submenus indirectly

			UINT pos = pCmdUI->m_nIndex;
			pCmdUI->m_pMenu->CheckMenuRadioItem( pos, pos, pos, MF_BYPOSITION );		// place radio checkmark
		}
	}

	bool ExpandVersionInfoTags( CCmdUI* pCmdUI )
	{
		ASSERT_PTR( pCmdUI );
		if ( pCmdUI->m_pMenu != NULL && NULL == pCmdUI->m_pSubMenu )		// a menu but not sub-menu (don't change submenus indirectly, wait for their expansion)
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

	void UpdateMenuUI( CWnd* pWindow, CMenu* pPopupMenu, bool autoMenuEnable /*= true*/ )
	{	// verbatim from CFrameWnd::OnInitMenuPopup
		CCmdUI state;

		state.m_pMenu = pPopupMenu;
		ASSERT( state.m_pOther == NULL );
		ASSERT( state.m_pParentMenu == NULL );

		// determine if menu is popup in top-level menu and set m_pOther to
		//  it if so (m_pParentMenu == NULL indicates that it is secondary popup)
		HMENU hParentMenu;
		if ( AfxGetThreadState()->m_hTrackingMenu == pPopupMenu->m_hMenu )
			state.m_pParentMenu = pPopupMenu;					// parent == child for tracking popup
		else if ( ( hParentMenu = ::GetMenu( pWindow->m_hWnd ) ) != NULL )
		{
			CWnd* pParent = pWindow->GetTopLevelParent();		// child windows don't have menus - need to go to the top

			if ( pParent != NULL &&
				( hParentMenu = ::GetMenu( pParent->m_hWnd ) ) != NULL )
			{
				int nIndexMax = ::GetMenuItemCount( hParentMenu );
				for ( int nIndex = 0; nIndex < nIndexMax; nIndex++ )
					if ( ::GetSubMenu( hParentMenu, nIndex ) == pPopupMenu->m_hMenu )	// popup found?
					{
						state.m_pParentMenu = CMenu::FromHandle( hParentMenu );			// m_pParentMenu is the containing menu
						break;
					}
			}
		}

		state.m_nIndexMax = pPopupMenu->GetMenuItemCount();
		for ( state.m_nIndex = 0; state.m_nIndex < state.m_nIndexMax; ++state.m_nIndex )
		{
			state.m_nID = pPopupMenu->GetMenuItemID( state.m_nIndex );
			if ( state.m_nID == 0 )
				continue;								// menu separator or invalid cmd - ignore it

			ASSERT( state.m_pOther == NULL );
			ASSERT( state.m_pMenu != NULL );
			if ( state.m_nID == (UINT)-1 )
			{
				// possibly a popup menu, route to first item of that popup
				state.m_pSubMenu = pPopupMenu->GetSubMenu( state.m_nIndex );
				if ( state.m_pSubMenu == NULL ||
					( state.m_nID = state.m_pSubMenu->GetMenuItemID( 0 ) ) == 0 || state.m_nID == (UINT)-1 )
					continue;	   // first item of popup can't be routed to

				state.DoUpdate( pWindow, FALSE );		// popups are never auto disabled
			}
			else
			{
				// normal menu item
				// auto enable/disable according to autoMenuEnable
				//	set and command is _not_ a system command.
				state.m_pSubMenu = NULL;
				state.DoUpdate( pWindow, !autoMenuEnable && state.m_nID < 0xF000 );
			}

			// adjust for menu deletions and additions
			UINT itemCount = pPopupMenu->GetMenuItemCount();
			if ( itemCount < state.m_nIndexMax )
			{
				state.m_nIndex -= ( state.m_nIndexMax - itemCount );
				while ( state.m_nIndex < itemCount && pPopupMenu->GetMenuItemID( state.m_nIndex ) == state.m_nID )
					++state.m_nIndex;
			}
			state.m_nIndexMax = itemCount;
		}
	}


	void UpdateControlsUI( CWnd* pParent, CWnd* pTargetWnd /*= NULL*/ )
	{
		if ( NULL == pTargetWnd )
			pTargetWnd = pParent;

		ASSERT_PTR( pParent->GetSafeHwnd() );
		ASSERT_PTR( pTargetWnd->GetSafeHwnd() );

		for ( CWnd* pCtrl = pParent->GetWindow( GW_CHILD ); pCtrl != NULL; pCtrl = pCtrl->GetNextWindow() )
			UpdateControlUI( pCtrl, pTargetWnd );
	}

	void UpdateControlsUI( CWnd* pParent, const UINT ctrlIds[], size_t count, CWnd* pTargetWnd /*= NULL*/ )
	{
		if ( NULL == pTargetWnd )
			pTargetWnd = pParent;

		ASSERT_PTR( pParent->GetSafeHwnd() );
		ASSERT_PTR( pTargetWnd->GetSafeHwnd() );

		for ( size_t i = 0; i != count; ++i )
			if ( CWnd* pCtrl = pParent->GetDlgItem( ctrlIds[ i ] ) )
				UpdateControlUI( pCtrl, pTargetWnd );
	}

	bool UpdateControlUI( CWnd* pCtrl, CWnd* pTargetWnd /*= NULL*/ )
	{
		if ( NULL == pTargetWnd )
			pTargetWnd = pCtrl->GetParent();

		ASSERT_PTR( pCtrl->GetSafeHwnd() );
		ASSERT_PTR( pTargetWnd->GetSafeHwnd() );

		CCmdUI ctrlState;

		ctrlState.m_nID = pCtrl->GetDlgCtrlID();
		if ( -1 == ui::ToCmdId( ctrlState.m_nID ) )
			return false;			// avoid anonymous statics and group-boxes

		ctrlState.m_pOther = pCtrl;
		return ctrlState.DoUpdate( pTargetWnd, FALSE ) != FALSE;
	}
}
