
#include "stdafx.h"
#include "UserInterfaceUtilities.h"
#include <afxpriv.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace ui
{
	CString GetWindowText( const CWnd* pWnd )
	{
		ASSERT_PTR( pWnd );
		CString text;
		pWnd->GetWindowText( text );
		return text;
	}

	void SetWindowText( CWnd* pWnd, const TCHAR* pText )
	{
		ASSERT_PTR( pWnd );
		AfxSetWindowText( pWnd->GetSafeHwnd(), pText );
	}

	CString GetControlText( const CWnd* pWnd, UINT ctrlId )
	{
		ASSERT_PTR( pWnd );
		CString text;
		pWnd->GetDlgItemText( ctrlId, text );
		return text;
	}

	void SetControlText( CWnd* pWnd, UINT ctrlId, const TCHAR* pText )
	{
		ASSERT_PTR( pWnd );
		ui::SetWindowText( pWnd->GetDlgItem( ctrlId ), pText );
	}

	HICON smartLoadIcon( LPCTSTR iconID, bool asLarge /*= true*/, HINSTANCE hResInst /*= AfxGetResourceHandle()*/ )
	{
		CSize iconSize( GetSystemMetrics( asLarge ? SM_CXICON : SM_CXSMICON ),
						GetSystemMetrics( asLarge ? SM_CYICON : SM_CYSMICON ) );
		HICON hIcon = (HICON)::LoadImage( hResInst, iconID, IMAGE_ICON, iconSize.cx, iconSize.cy, LR_DEFAULTCOLOR );

		return hIcon;
	}

	bool smartEnableWindow( CWnd* pWnd, bool enable /*= true*/ )
	{
		ASSERT_PTR( pWnd );
		if ( !( pWnd->GetStyle() & WS_DISABLED ) == enable )
			return false;
		pWnd->EnableWindow( enable );
		return true;
	}

	void smartEnableControls( CWnd* pDlg, const UINT* pCtrlIds, size_t ctrlCount, bool enable /*= true*/ )
	{
		for ( unsigned int i = 0; i != ctrlCount; ++i )
			smartEnableWindow( pDlg->GetDlgItem( pCtrlIds[ i ] ), enable );
	}

	void commitMenuEnabling( CWnd& targetWnd, CMenu& popupMenu ) // check the enabled state of various menu items
	{
		CCmdUI enabler;
		HMENU hParentMenu;

		enabler.m_pMenu = &popupMenu;
		ASSERT( enabler.m_pOther == NULL && enabler.m_pParentMenu == NULL );
		// Determine if menu is popup in top-level menu and set m_pOther to
		//  it if so (m_pParentMenu == NULL indicates that it is secondary popup)
		if ( AfxGetThreadState()->m_hTrackingMenu == popupMenu.m_hMenu )
			enabler.m_pParentMenu = &popupMenu;    // parent == child for tracking popup
		else if ( ( hParentMenu = ::GetMenu( targetWnd ) ) != NULL )
		{
			CWnd* pParent = targetWnd.GetTopLevelParent();

			// Child windows don't have menus -- need to go to the top!
			if ( pParent != NULL &&
				 ( hParentMenu = ::GetMenu( pParent->m_hWnd ) ) != NULL )
			{
				for ( int nIndex = 0, nIndexMax = ::GetMenuItemCount( hParentMenu ); nIndex < nIndexMax; nIndex++ )
				{
					if ( ::GetSubMenu( hParentMenu, nIndex ) == popupMenu.m_hMenu )
					{
						// When popup is found, m_pParentMenu is containing menu
						enabler.m_pParentMenu = CMenu::FromHandle( hParentMenu );
						break;
					}
				}
			}
		}

		enabler.m_nIndexMax = popupMenu.GetMenuItemCount();
		for ( enabler.m_nIndex = 0; enabler.m_nIndex < enabler.m_nIndexMax; enabler.m_nIndex++ )
		{
			enabler.m_nID = popupMenu.GetMenuItemID( enabler.m_nIndex );
			if ( enabler.m_nID == 0 )
				continue; // Menu separator or invalid cmd - ignore it

			ASSERT( enabler.m_pOther == NULL && enabler.m_pMenu != NULL );
			if ( enabler.m_nID == (UINT)-1 )
			{	// Possibly a popup menu, route to first item of that popup
				enabler.m_pSubMenu = popupMenu.GetSubMenu( enabler.m_nIndex );
				if ( enabler.m_pSubMenu == NULL ||
					 ( enabler.m_nID = enabler.m_pSubMenu->GetMenuItemID( 0 ) ) == 0 ||
					 enabler.m_nID == (UINT)-1 )
					// First item of popup can't be routed to.
					continue;
				// Popups are never auto disabled
				enabler.DoUpdate( &targetWnd, FALSE );
			}
			else
			{	// Normal menu item
				enabler.m_pSubMenu = NULL;
				enabler.DoUpdate( &targetWnd, enabler.m_nID < 0xF000 );
			}
			// Adjust for menu deletions and additions:
			UINT nCount = popupMenu.GetMenuItemCount();

			if ( nCount < enabler.m_nIndexMax )
			{
				enabler.m_nIndex -= ( enabler.m_nIndexMax - nCount );
				while ( enabler.m_nIndex < nCount && popupMenu.GetMenuItemID( enabler.m_nIndex ) == enabler.m_nID )
					enabler.m_nIndex++;
			}
			enabler.m_nIndexMax = nCount;
		}
	}

	void updateControlsUI( CWnd& targetWnd )
	{
		for ( CWnd* ctrl = targetWnd.GetWindow( GW_CHILD ); ctrl != NULL; ctrl = ctrl->GetNextWindow() )
		{
			CCmdUI commandUI;

			commandUI.m_nID = ctrl->GetDlgCtrlID();
			if ( commandUI.m_nID != UINT_MAX )
			{
				commandUI.m_pOther = ctrl;
				commandUI.DoUpdate( &targetWnd, FALSE );
			}
		}
	}

	CString GetCommandText( CCmdUI& rCmdUI )
	{
		CString commandText;

		if ( rCmdUI.m_pMenu != NULL )
			rCmdUI.m_pMenu->GetMenuString( rCmdUI.m_nID, commandText, MF_BYCOMMAND );
		else
		{
			ASSERT_PTR( rCmdUI.m_pOther );
			rCmdUI.m_pOther->GetWindowText( commandText );
		}

		return commandText;
	}


	// aligns 'restDest' to 'rectFixed' according to horizontal and vertical alignment flags

	CRect& alignRect( CRect& restDest, const CRect& rectFixed, HorzAlign horzAlign /*= Horz_AlignCenter*/,
					  VertAlign vertAlign /*= Vert_AlignCenter*/, bool limitDest /*= false*/ )
	{
		CSize offset( 0, 0 );

		switch ( horzAlign )
		{
			case Horz_AlignLeft:
				offset.cx = rectFixed.left - restDest.left;
				break;
			case Horz_AlignCenter:
				offset.cx = rectFixed.left - restDest.left + ( rectFixed.Width() - restDest.Width() ) / 2;
				break;
			case Horz_AlignRight:
				offset.cx = rectFixed.right - restDest.Width() - restDest.left;
				break;
			case Horz_NoAlign:
				break;
			default:
				ASSERT( false );
		}
		switch ( vertAlign )
		{
			case Vert_AlignTop:
				offset.cy = rectFixed.top - restDest.top;
				break;
			case Vert_AlignCenter:
				offset.cy = rectFixed.top - restDest.top + ( rectFixed.Height() - restDest.Height() ) / 2;
				break;
			case Vert_AlignBottom:
				offset.cy = rectFixed.bottom - restDest.Height() - restDest.top;
				break;
			case Vert_NoAlign:
				break;
			default:
				ASSERT( false );
		}
		restDest.OffsetRect( offset );
		if ( limitDest && !restDest.IsRectEmpty() )
			restDest &= rectFixed;
		return restDest;
	}

	// Centers 'restDest' rectangle in 'rectFixed' rectangle vertically and/or horizontally
	// depending on 'horizontally' and 'vertically' parameters.
	// If 'limitDest' is true it also limits 'restDest' bounds to 'rectFixed'.

	CRect& centerRect( CRect& restDest, const CRect& rectFixed, bool horizontally /*= true*/,
					   bool vertically /*= true*/, bool limitDest /*= false*/,
					   const CSize& addOffset /*= CSize( 0, 0 )*/ )
	{
		CPoint offsetBy = rectFixed.TopLeft() - restDest.TopLeft() + MulDiv_Size( rectFixed.Size() - restDest.Size() + addOffset, 1, 2 );

		if ( !horizontally )
			offsetBy.x = 0;
		if ( !vertically )
			offsetBy.y = 0;
		restDest.OffsetRect( offsetBy );
		if ( limitDest && !restDest.IsRectEmpty() )
			restDest &= rectFixed;
		return restDest;
	}

	// Shifts vertically and/or horizontally 'restDest' if necessary in order to fit inside 'rectFixed'

	bool ensureVisibleRect( CRect& restDest, const CRect& rectFixed, bool horizontally /*= true*/,
							bool vertically /*= true*/ )
	{
		if ( restDest.left >= rectFixed.left && restDest.top >= rectFixed.top &&
			 restDest.right <= rectFixed.right && restDest.bottom <= rectFixed.bottom )
			return false;		// No 'restDest' overflow.

		CPoint offset( 0, 0 );

		if ( horizontally )
			if ( restDest.Width() > rectFixed.Width() )
				offset.x = rectFixed.left - restDest.left;
			else
				if ( restDest.left < rectFixed.left )
					offset.x = rectFixed.left - restDest.left;
				else if ( restDest.right > rectFixed.right )
					offset.x = rectFixed.right - restDest.right;
		if ( vertically )
			if ( restDest.Height() > rectFixed.Height() )
				offset.y = rectFixed.top - restDest.top;
			else
				if ( restDest.top < rectFixed.top )
					offset.y = rectFixed.top - restDest.top;
				else if ( restDest.bottom > rectFixed.bottom )
					offset.y = rectFixed.bottom - restDest.bottom;
		if ( offset == CPoint( 0, 0 ) )
			return false;		// No adjustment.
		restDest += offset;
		return true;
	}

	// If point is within the bounds of rect it returns true, otherwise it limits
	// point so won't exceed rect bounds and returns false.
	bool ensurePointInRect( POINT& point, const RECT& rect )
	{
		if ( point.x >= rect.left && point.x <= rect.right && point.y >= rect.top && point.y <= rect.bottom )
			return true;		// point in the bounds of specified rectangle -> don't adjust it

		if ( point.x < rect.left )
			point.x = rect.left;
		else if ( point.x > rect.right )
			point.x = rect.right;

		if ( point.y < rect.top )
			point.y = rect.top;
		else if ( point.y > rect.bottom )
			point.y = rect.bottom;

		return false;			// point limited to the bounds of rect
	}

} // namespace ui
