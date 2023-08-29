
#include "pch.h"
#include "MenuUtilities.h"
#include "CmdUpdate.h"
#include "ImageStore.h"
#include "WndUtils.h"
#include "utl/Algorithms.h"

#ifdef _DEBUG
#include "FlagTags.h"
#define new DEBUG_NEW
#endif


namespace ui
{
	bool LoadPopupMenu( OUT CMenu* pContextMenu, UINT menuResId, const CPopupIndexPath& popupIndexPath, UseMenuImages useMenuImages /*= NormalMenuImages*/, OUT std::tstring* pOutPopupText /*= nullptr*/ )
	{
		ASSERT_PTR( pContextMenu );
		REQUIRE( popupIndexPath.GetDepth() > 0 );

		pContextMenu->DestroyMenu();

		CMenu menuBar;
		VERIFY( menuBar.LoadMenu( menuResId ) );

		CMenu* pParentMenu = &menuBar;
		size_t depthLevel = 0;
		int popupIndex = popupIndexPath.GetPopupIndexAt( depthLevel );

		while ( ++depthLevel != popupIndexPath.GetDepth() && pParentMenu != nullptr )
		{
			int index = popupIndexPath.GetPopupIndexAt( depthLevel );

			if ( -1 == index )
				break;				// no more depth requested

			pParentMenu = pParentMenu->GetSubMenu( popupIndex );
			popupIndex = index;
		}

		if ( pParentMenu != nullptr && popupIndex >= 0 )
		{
			if ( pOutPopupText != nullptr )
			{
				CString popupText;
				pParentMenu->GetMenuString( popupIndex, popupText, MF_BYPOSITION );
				*pOutPopupText = popupText.GetString();
			}

			if ( HMENU hSubMenu = ::GetSubMenu( *pParentMenu, popupIndex ) )
			{
				pContextMenu->Attach( hSubMenu );
				VERIFY( pParentMenu->RemoveMenu( popupIndex, MF_BYPOSITION ) );

				if ( useMenuImages != NoMenuImages )
					SetMenuImages( pContextMenu, CheckedMenuImages == useMenuImages );		// set shared bitmap images
				return true;
			}
			else
				ENSURE( false );		// the requested popup is not a sub-menu
		}
		else
			ENSURE( false );			// couldn't locate the requested popup

		return false;
	}


	bool SetMenuImages( OUT CMenu* pMenu, bool useCheckedBitmaps /*= false*/, ui::IImageStore* pImageStore /*= nullptr*/ )
	{
		REQUIRE( ::IsMenu( pMenu->GetSafeHmenu() ) );

		if ( nullptr == pImageStore )
			pImageStore = ui::GetImageStoresSvc();
		if ( nullptr == pImageStore )
			return false;

		MENUITEMINFO info; utl::ZeroWinStruct( &info );
		info.fMask = MIIM_FTYPE | MIIM_SUBMENU | MIIM_ID;

		for ( UINT i = 0, count = pMenu->GetMenuItemCount(); i != count; ++i )
			if ( pMenu->GetMenuItemInfo( i, &info, true ) )
				if ( info.hSubMenu != nullptr )
					SetMenuImages( CMenu::FromHandle( info.hSubMenu ), useCheckedBitmaps, pImageStore );
				else if ( !HasFlag( info.fType, MFT_SEPARATOR | MFT_OWNERDRAW ) )
				{
					ASSERT( info.wID != 0 && info.wID != UINT_MAX );

					std::pair<CBitmap*, CBitmap*> bitmaps = pImageStore->RetrieveMenuBitmaps( info.wID, useCheckedBitmaps );
					if ( bitmaps.first != nullptr || bitmaps.second != nullptr )
						pMenu->SetMenuItemBitmaps( i, MF_BYPOSITION, bitmaps.first, bitmaps.second );
				}

		return true;
	}

	bool SetMenuItemImage( OUT CMenu* pMenu, const CMenuItemRef& itemRef, UINT iconId /*= 0*/, bool useCheckedBitmaps /*= false*/, ui::IImageStore* pImageStore /*= nullptr*/ )
	{
		REQUIRE( ::IsMenu( pMenu->GetSafeHmenu() ) );

		if ( nullptr == pImageStore )
			pImageStore = ui::GetImageStoresSvc();

		std::pair<CBitmap*, CBitmap*> bitmaps = pImageStore->RetrieveMenuBitmaps( 0 == iconId ? itemRef.GetCmdId() : iconId, useCheckedBitmaps );
		if ( bitmaps.first != nullptr || bitmaps.second != nullptr )
		{
			pMenu->SetMenuItemBitmaps( itemRef.m_itemRef, itemRef.m_refFlags, bitmaps.first, bitmaps.second );
			return true;
		}

		return false;
	}

	int TrackPopupMenu( CMenu& rMenu, CWnd* pTargetWnd, CPoint screenPos, UINT trackFlags /*= TPM_RIGHTBUTTON*/, const RECT* pExcludeRect /*= nullptr*/ )
	{
		int cmdId = 0;

		if ( UseMfcMenuManager() && CScopedTrackMfcPopupMenu::GetTrackMfcPopup() )
		{
			cmdId = TrackMfcPopupMenu( rMenu.GetSafeHmenu(), pTargetWnd, screenPos, !HasFlag( trackFlags, TPM_RETURNCMD ) );
		}
		else
		{
			AdjustMenuTrackPos( screenPos, nullptr );

			TPMPARAMS excludeParams; utl::ZeroWinStruct( &excludeParams );

			if ( pExcludeRect != nullptr )			// pExcludeRect is ignored by TrackPopupMenu()
				excludeParams.rcExclude = *pExcludeRect;

			cmdId = ui::ToIntCmdId( rMenu.TrackPopupMenuEx( trackFlags, screenPos.x, screenPos.y, pTargetWnd, pExcludeRect != nullptr ? &excludeParams : nullptr ) );
		}

		return cmdId;
	}

	int TrackPopupMenuAlign( CMenu& rMenu, CWnd* pTargetWnd, const RECT& excludeRect, PopupAlign popupAlign /*= DropDown*/,
							 UINT trackFlags /*= TPM_RIGHTBUTTON*/ )
	{
		if ( !HasFlag( trackFlags, TPM_RIGHTALIGN | TPM_BOTTOMALIGN ) )
			trackFlags |= GetAlignTrackFlags( popupAlign );

		return TrackPopupMenu( rMenu, pTargetWnd, GetAlignTrackPos( popupAlign, excludeRect ), trackFlags, &excludeRect );
	}


	int TrackContextMenu( UINT menuResId, const CPopupIndexPath& popupIndexPath, CWnd* pTargetWnd, CPoint screenPos, UINT trackFlags /*= TPM_RIGHTBUTTON*/, const RECT* pExcludeRect /*= nullptr*/ )
	{
		CMenu contextMenu;

		ui::LoadPopupMenu( &contextMenu, menuResId, popupIndexPath );
		return ui::TrackPopupMenu( contextMenu, pTargetWnd, screenPos, trackFlags, pExcludeRect );
	}



	int TrackMfcPopupMenu( HMENU hPopupMenu, CWnd* pTargetWnd, CPoint screenPos, bool sendCommand /*= true*/ )
	{
		UINT cmdId = 0;

		REQUIRE( ::IsMenu( hPopupMenu ) );
		AdjustMenuTrackPos( screenPos, pTargetWnd );

		if ( UseMfcMenuManager() )
		{
			HWND hFocusWnd = ::GetFocus();

			cmdId = afxContextMenuManager->TrackPopupMenu( hPopupMenu, screenPos.x, screenPos.y, pTargetWnd );

			ui::TakeFocus( hFocusWnd );
		}
		else
		{
			CMenu* pPopupMenu = CMenu::FromHandle( hPopupMenu );

			// not using TPM_NONOTIFY for proper WM_INITMENUPOPUP menu updates
			cmdId = pPopupMenu->TrackPopupMenu( TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_RETURNCMD, screenPos.x, screenPos.y, pTargetWnd );
		}

		if ( sendCommand && cmdId != 0 )
			SendCommand( pTargetWnd->GetSafeHwnd(), cmdId );

		return ui::ToIntCmdId( cmdId );
	}

	int TrackMfcContextMenu( UINT menuResId, const CPopupIndexPath& popupIndexPath, CWnd* pTargetWnd, CPoint screenPos, bool sendCommand /*= true*/ )
	{
		CMenu contextMenu;

		ui::LoadMfcPopupMenu( &contextMenu, menuResId, popupIndexPath );
		return TrackMfcPopupMenu( contextMenu, pTargetWnd, screenPos, sendCommand );
	}


	CWnd* AutoTargetWnd( CWnd* pTargetWnd )
	{
		CWnd* pCmdTargetWnd = pTargetWnd;

		if ( pTargetWnd != nullptr && HasFlag( pTargetWnd->GetStyle(), WS_CHILD ) )
		{	// pTargetWnd is not a dialog
			pCmdTargetWnd = pTargetWnd->GetParentFrame();

			if ( nullptr == pCmdTargetWnd )
				pCmdTargetWnd = pTargetWnd->GetParent();

			if ( nullptr == pCmdTargetWnd || !pCmdTargetWnd->IsChild( pTargetWnd ) )
				pCmdTargetWnd = pTargetWnd;
		}

		ASSERT_PTR( pCmdTargetWnd );
		return pCmdTargetWnd;
	}

	bool AdjustMenuTrackPos( CPoint& rScreenPos, CWnd* pWndAlign )
	{
		if ( -1 == rScreenPos.x && -1 == rScreenPos.y )
		{
			if ( nullptr == pWndAlign )
				pWndAlign = CWnd::GetFocus();

			if ( pWndAlign != nullptr )
			{
				CRect targetRect;
				pWndAlign->GetWindowRect( &targetRect );

				rScreenPos.x = targetRect.left + 1;			// skip the border of target window
				rScreenPos.y = targetRect.bottom;
			}
			else
				::GetCursorPos( &rScreenPos );

			return true;
		}

		return false;
	}

	DWORD GetAlignTrackFlags( PopupAlign popupAlign )
	{
		switch ( popupAlign )
		{
			case DropDown:	return TPM_TOPALIGN;
			case DropUp:	return TPM_BOTTOMALIGN;
			case DropLeft:	return TPM_RIGHTALIGN;
		}
		return TPM_LEFTALIGN;
	}

	CPoint GetAlignTrackPos( PopupAlign popupAlign, const RECT& excludeRect )
	{
		switch ( popupAlign )
		{
			default: ASSERT( false );
			case DropDown:
				return CPoint( excludeRect.left, excludeRect.bottom );
			case DropRight:
				return CPoint( excludeRect.right, excludeRect.top );
			case DropUp:
			case DropLeft:
				return CPoint( excludeRect.left, excludeRect.top );
		}
	}
}


namespace ui
{
	bool CScopedTrackMfcPopupMenu::s_trackMfcPopup = false;
}


namespace ui
{
	UINT GetMenuItemType( HMENU hMenu, UINT item, bool byPos )
	{
		MENUITEMINFO info; utl::ZeroWinStruct( &info );

		info.fMask = MIIM_TYPE;

		if ( !::GetMenuItemInfo( hMenu, item, byPos, &info ) )
			return 0;

		return (DWORD_PTR)info.fType;
	}

	void* GetMenuItemData( HMENU hMenu, UINT item, bool byPos /*= true*/ )
	{
		MENUITEMINFO info; utl::ZeroWinStruct( &info );

		info.fMask = MIIM_DATA;

		if ( !::GetMenuItemInfo( hMenu, item, byPos, &info ) )		// menu item not present
			return nullptr;

		return reinterpret_cast<void*>( info.dwItemData );
	}

	bool SetMenuItemData( HMENU hMenu, UINT item, const void* pItemData, bool byPos /*= true*/ )
	{
		MENUITEMINFO info; utl::ZeroWinStruct( &info );

		info.fMask = MIIM_DATA;
		info.dwItemData = reinterpret_cast<DWORD_PTR>( pItemData );

		return ::SetMenuItemInfo( hMenu, item, byPos, &info ) != FALSE;
	}


	int FindMenuItemIndex( HMENU hMenu, UINT itemId, unsigned int iFirst /*= 0*/ )
	{
		ASSERT_PTR( hMenu );

		for ( unsigned int count = ::GetMenuItemCount( hMenu ); iFirst < count; ++iFirst )
			if ( ::GetMenuItemID( hMenu, iFirst ) == itemId )
				return iFirst;

		return -1;
	}

	int FindAfterMenuItemIndex( HMENU hMenu, UINT itemId, unsigned int iFirst /*= 0*/ )
	{
		int atIndex = FindMenuItemIndex( hMenu, itemId, iFirst );
		if ( -1 == atIndex )
			return atIndex;

		return atIndex + 1;			// the position just after
	}

	int ContainsMenuItem( HMENU hMenu, UINT itemId, RecursionDepth depth /*= Shallow*/ )
	{
		if ( Shallow == depth )
			return ui::FindMenuItemIndex( hMenu, itemId ) != -1;

		return ::GetMenuState( hMenu, itemId, MF_BYCOMMAND ) != UINT_MAX;		// this finds deep menu items (searching in sub-menus)
	}

	bool ReplaceMenuItemWithPopup( CMenu* pPopupMenu, UINT itemId, UINT menuResId, const CPopupIndexPath& popupIndexPath, UseMenuImages useMenuImages /*= ui::NoMenuImages*/ )
	{
		int itemPos = ui::FindMenuItemIndex( pPopupMenu->GetSafeHmenu(), itemId );

		if ( -1 == itemPos )
			return false;		// item not found

		CMenu subMenu;
		std::tstring popupText = ui::GetMenuItemText( pPopupMenu, itemPos, MF_BYPOSITION );
		std::tstring* pOutPopupText = ( popupText.empty() || '<' == popupText[0] ) ? &popupText : nullptr;		// use standard sub-menu item text if existing item has placeholder text

		return
			ui::LoadPopupMenu( &subMenu, menuResId, popupIndexPath, useMenuImages, pOutPopupText )
			&& pPopupMenu->ModifyMenu( itemPos, MF_POPUP | MF_BYPOSITION, reinterpret_cast<UINT_PTR>( subMenu.Detach() ), popupText.c_str() ) != FALSE;
	}


	HMENU FindMenuItemIndex( OUT int* pIndex, HMENU hMenu, UINT itemId, RecursionDepth depth /*= Deep*/ )
	{
		REQUIRE( ::IsMenu( hMenu ) );
		ASSERT_PTR( pIndex );

		MENUITEMINFO info; utl::ZeroWinStruct( &info );
		info.fMask = MIIM_FTYPE | MIIM_SUBMENU | MIIM_ID;

		for ( UINT i = 0, count = ::GetMenuItemCount( hMenu ); i != count; ++i )
			if ( ::GetMenuItemInfo( hMenu, i, true, &info ) )
				if ( info.hSubMenu != nullptr )
				{
					if ( Deep == depth )
						if ( HMENU hFoundSubMenu = FindMenuItemIndex( pIndex, info.hSubMenu, itemId, depth ) )
							return hFoundSubMenu;
				}
				else if ( itemId == info.wID )
				{
					*pIndex = i;
					return hMenu;
				}

		*pIndex = -1;
		return nullptr;
	}

	HMENU FindFirstMenuCommand( OUT UINT* pCmdId, HMENU hMenu, RecursionDepth depth /*= Deep*/ )
	{
		REQUIRE( ::IsMenu( hMenu ) );
		ASSERT_PTR( pCmdId );

		MENUITEMINFO info; utl::ZeroWinStruct( &info );
		info.fMask = MIIM_FTYPE | MIIM_SUBMENU | MIIM_ID;

		for ( UINT i = 0, count = ::GetMenuItemCount( hMenu ); i != count; ++i )
			if ( ::GetMenuItemInfo( hMenu, i, true, &info ) )
				if ( info.hSubMenu != nullptr )
				{
					if ( Deep == depth )
						if ( HMENU hFoundSubMenu = FindFirstMenuCommand( pCmdId, info.hSubMenu, depth ) )
							return hFoundSubMenu;
				}
				else if ( !HasFlag( info.fType, MFT_SEPARATOR ) )
				{
					*pCmdId = info.wID;
					return hMenu;
				}

		*pCmdId = 0;
		return nullptr;
	}

	UINT GetTotalCmdCount( HMENU hMenu, RecursionDepth depth /*= Deep*/ )
	{
		REQUIRE( ::IsMenu( hMenu ) );
		UINT cmdCount = 0;

		MENUITEMINFO info; utl::ZeroWinStruct( &info );
		info.fMask = MIIM_FTYPE | MIIM_SUBMENU | MIIM_ID;

		for ( UINT i = 0, count = ::GetMenuItemCount( hMenu ); i != count; ++i )
			if ( ::GetMenuItemInfo( hMenu, i, true, &info ) )
				if ( info.hSubMenu != nullptr )
				{
					if ( Deep == depth )
						cmdCount += GetTotalCmdCount( info.hSubMenu, Deep );
				}
				else if ( !HasFlag( info.fType, MFT_SEPARATOR ) )
					++cmdCount;

		return cmdCount;
	}

	void QueryMenuItemIds( std::vector<UINT>& rItemIds, HMENU hMenu, RecursionDepth depth /*= Deep*/ )
	{
		REQUIRE( ::IsMenu( hMenu ) );

		MENUITEMINFO info; utl::ZeroWinStruct( &info );
		info.fMask = MIIM_FTYPE | MIIM_SUBMENU | MIIM_ID;

		for ( UINT i = 0, count = ::GetMenuItemCount( hMenu ); i != count; ++i )
			if ( ::GetMenuItemInfo( hMenu, i, true, &info ) )
				if ( info.hSubMenu != nullptr )
				{
					if ( Deep == depth )
						QueryMenuItemIds( rItemIds, info.hSubMenu, depth );
				}
				else if ( !HasFlag( info.fType, MFT_SEPARATOR ) )
					utl::AddUnique( rItemIds, info.wID );
	}

	HMENU CloneMenu( HMENU hSrcMenu )
	{
		REQUIRE( ::IsMenu( hSrcMenu ) );

		CMenu destMenu;
		destMenu.CreatePopupMenu();

		for ( int i = 0, count = ::GetMenuItemCount( hSrcMenu ); i != count; ++i )
		{
			MENUITEMINFO_BUFF info;
			if ( info.GetMenuItemInfo( hSrcMenu, i ) )
			{
				if ( info.hSubMenu != nullptr )
					info.hSubMenu = ui::CloneMenu( info.hSubMenu );		// clone the sub-menu inplace

				VERIFY( destMenu.InsertMenuItem( i, &info, TRUE ) );

				if ( !info.IsSeparator() )
					SetMenuItemImage( &destMenu, info.wID );
			}
			else
				ASSERT( false );		// invalid item?
		}

		return destMenu.Detach();
	}

	size_t CopyMenuItems( OUT CMenu* pDestMenu, unsigned int destIndex, const CMenu* pSrcMenu, const std::vector<UINT>* pSrcIds /*= nullptr*/ )
	{
		REQUIRE( ::IsMenu( pDestMenu->GetSafeHmenu() ) );
		size_t copiedCount = 0;

		for ( UINT i = 0, srcCount = pSrcMenu->GetMenuItemCount(); i != srcCount; ++i )
		{
			UINT srcId = pSrcMenu->GetMenuItemID( i );

			if ( nullptr == pSrcIds || ( 0 == srcId || std::find( pSrcIds->begin(), pSrcIds->end(), srcId ) != pSrcIds->end() ) )
			{
				MENUITEMINFO_BUFF info;
				if ( info.GetMenuItemInfo( pSrcMenu->GetSafeHmenu(), i ) )
				{
					VERIFY( pDestMenu->InsertMenuItem( destIndex, &info, TRUE ) );

					if ( !info.IsSeparator() )
						SetMenuItemImage( pDestMenu, info.wID );

					++copiedCount;
					++destIndex;
				}
				else
					ASSERT( false );
			}
		}

		return copiedCount;
	}

	void DeleteMenuItem( OUT CMenu* pMenu, UINT itemId )
	{
		REQUIRE( ::IsMenu( pMenu->GetSafeHmenu() ) );
		ASSERT( itemId != 0 );

		if ( pMenu->DeleteMenu( itemId, MF_BYCOMMAND ) )
			CleanupMenuSeparators( pMenu );
	}

	size_t DeleteMenuItems( OUT CMenu* pMenu, const UINT* pItemIds, size_t itemCount )
	{
		REQUIRE( ::IsMenu( pMenu->GetSafeHmenu() ) );
		ASSERT_PTR( pItemIds );

		size_t delCount = 0;
		for ( UINT i = 0; i != itemCount; ++i )
		{
			ASSERT( pItemIds[ i ] != 0 );
			if ( pMenu->DeleteMenu( pItemIds[ i ], MF_BYCOMMAND ) )
				++delCount;
		}

		return delCount + CleanupMenuSeparators( pMenu );
	}

	size_t CleanupMenuDuplicates( OUT CMenu* pMenu )
	{
		REQUIRE( ::IsMenu( pMenu->GetSafeHmenu() ) );

		size_t delCount = 0;

		// first delete any duplicate commands (no separators)
		for ( int i = 0, count = pMenu->GetMenuItemCount(); i != count; ++i )
		{
			UINT itemId = pMenu->GetMenuItemID( i );

			if ( UINT_MAX == itemId )	// sub-menu
			{
				if ( CMenu* pSubMenu = pMenu->GetSubMenu( i ) )
					delCount += CleanupMenuDuplicates( pSubMenu );
			}
			else if ( itemId != 0 )		// normal command, no separator
			{
				for ( ;; )
				{
					UINT iDuplicate = FindMenuItemIndex( pMenu->GetSafeHmenu(), itemId, i + 1 );

					if ( iDuplicate != UINT_MAX )								// found a duplicate?
					{
						pMenu->DeleteMenu( iDuplicate, MF_BYPOSITION );
						++delCount;
					}
					else
						break;
				}
			}
		}

		return delCount + CleanupMenuSeparators( pMenu );
	}


	size_t CleanupMenuSeparators( OUT CMenu* pMenu )
	{
		size_t delCount = 0;
		MENUITEMINFO info; utl::ZeroWinStruct( &info );

		info.fMask = MIIM_FTYPE | MIIM_SUBMENU | MIIM_ID;

		// delete duplicate separators in reverse order
		bool prevSep = false;
		for ( UINT count = pMenu->GetMenuItemCount(), i = count; i-- != 0; )
			if ( pMenu->GetMenuItemInfo( i, &info, true ) )
				if ( HasFlag( info.fType, MFT_SEPARATOR ) )
				{
					if ( prevSep || 0 == i || ( count - 1 ) == i )		// duplicate, leading, or trailing separator
					{
						pMenu->DeleteMenu( i, MF_BYPOSITION );
						++delCount;
					}
					prevSep = true;
				}
				else
				{
					prevSep = false;

					if ( info.hSubMenu != nullptr )
						delCount += CleanupMenuSeparators( CMenu::FromHandle( info.hSubMenu ) );
				}

		if ( prevSep )			// a leftover leading separator?  it can happen in case of a leading multiple separator sequence
			pMenu->DeleteMenu( 0, MF_BYPOSITION );

		return delCount;
	}


	bool JoinMenuItems( OUT CMenu* pDestMenu, const CMenu* pSrcMenu, MenuInsert menuInsert /*= AppendSrc*/, bool addSep /*= true*/, UseMenuImages useMenuImages /*= NormalMenuImages*/ )
	{
		REQUIRE( ::IsMenu( pDestMenu->GetSafeHmenu() ) );
		REQUIRE( ::IsMenu( pSrcMenu->GetSafeHmenu() ) );

		if ( 0 == pSrcMenu->GetMenuItemCount() )
			return false;

		switch ( menuInsert )
		{
			case PrependSrc:
				if ( addSep )
					pDestMenu->InsertMenu( 0, MF_SEPARATOR | MF_BYPOSITION );
				CopyMenuItems( pDestMenu, 0, pSrcMenu );
				break;
			case AppendSrc:
				if ( addSep )
					pDestMenu->InsertMenu( pDestMenu->GetMenuItemCount(), MF_SEPARATOR | MF_BYPOSITION );
				CopyMenuItems( pDestMenu, pDestMenu->GetMenuItemCount(), pSrcMenu );
				break;
		}

		CleanupMenuDuplicates( pDestMenu );
		CleanupMenuSeparators( pDestMenu );

		if ( useMenuImages != NoMenuImages )
			SetMenuImages( pDestMenu, CheckedMenuImages == useMenuImages );

		return true;
	}


	bool EnsureDeepValidMenu( HMENU hMenu, unsigned int depth /*= 0*/ )
	{
		ASSERT_PTR( hMenu );

		if ( !::IsMenu( hMenu ) )
			return false;

		for ( UINT i = 0, count = ::GetMenuItemCount( hMenu ); i != count; ++i )
			switch ( ::GetMenuItemID( hMenu, i ) )
			{
				case 0:				// separator
					break;
				case UINT_MAX:		// sub-menu?
					if ( HMENU hSubMenu = ::GetSubMenu( hMenu, i ) )
						if ( !EnsureDeepValidMenu( hSubMenu, depth + 1 ) )
						{
							TCHAR text[ 256 ] = { 0 };
							::GetMenuString( hMenu, i, text, COUNT_OF( text ), MF_BYPOSITION );

							std::tstring indentPrefix( depth, _T(' ') );

							TRACE( _T("%s- Invalid sub-menu item: hMenu=0x%08x  itemPos=%d  itemText='%s'  hSubMenu=0x%08x  depth=%d\n"), indentPrefix.c_str(), hMenu, i, text, hSubMenu, depth );
							return false;
						}
					break;
			}

		return true;
	}
}


namespace ui
{
	HWND FindMenuWindowFromPoint( CPoint screenPos /*= ui::GetCursorPos()*/ )
	{
		if ( HWND hWnd = ::WindowFromPoint( screenPos ) )
			if ( ui::IsMenuWnd( hWnd ) )
				return hWnd;

		return nullptr;
	}

	bool IsHiliteMenuItem( HMENU hMenu, int itemPos )
	{
		ASSERT_PTR( hMenu );

		MENUITEMINFO info; utl::ZeroWinStruct( &info );
		info.fMask = MIIM_FTYPE | MIIM_STATE | MIIM_SUBMENU | MIIM_ID;

		if ( ::GetMenuItemInfo( hMenu, itemPos, true, &info ) )
			if ( !HasFlag( info.fType, MFT_SEPARATOR ) && nullptr == info.hSubMenu )
				if ( !HasFlag( info.fState, MFS_GRAYED | MFS_DISABLED ) )
					return HasFlag( info.fState, MF_HILITE );

		return false;
	}

	void InvalidateMenuWindow( void )
	{
		if ( HWND hMenuWnd = FindMenuWindowFromPoint() )
			::InvalidateRect( hMenuWnd, nullptr, TRUE );
	}


	bool ScrollVisibleMenuItem( HWND hTrackWnd, HMENU hMenu, UINT hiliteId )
	{
		ASSERT( hiliteId != 0 );
		ASSERT( ::IsWindow( hTrackWnd ) );
		ASSERT( ::IsMenu( hMenu ) );

		// post arrow down for non-separator items until reaching the hiliteId item; scrolls the menu if necessary
		if ( ::GetMenuState( hMenu, hiliteId, MF_BYCOMMAND ) != UINT_MAX )		// item exists in popup?
			for ( UINT i = 0, count = ::GetMenuItemCount( hMenu ); i != count; ++i )
				if ( UINT itemId = ::GetMenuItemID( hMenu, i ) )				// not a separator?
				{	// non-separator item, simulate arrow-down key
					::PostMessage( hTrackWnd, WM_KEYDOWN, VK_DOWN, 0 );

					if ( itemId == hiliteId )
						return true;
				}

		return false;
	}

	bool HoverOnMenuItem( HWND hTrackWnd, HMENU hMenu, UINT hiliteId )
	{	// don't call this directly: works when called delayed, e.g. through a ui::PostCall when handling WM_INITMENUPOPUP
		ASSERT( hiliteId != 0 );
		ASSERT_PTR( hTrackWnd );
		ASSERT( ::IsMenu( hMenu ) );

		int itemPos = ui::FindMenuItemIndex( hMenu, hiliteId );
		if ( itemPos != -1 )
		{
			CRect itemRect;
			if ( ::GetMenuItemRect( hTrackWnd, hMenu, itemPos, &itemRect ) )
				if ( !itemRect.IsRectEmpty() )
				{
					CPoint itemPoint = itemRect.CenterPoint();
					CRect workspaceRect = ui::FindMonitorRect( hTrackWnd, ui::Workspace );
					if ( workspaceRect.PtInRect( itemPoint ) )
					{	// item is visible, no menu scrolling is required, so we're good
						::SetCursorPos( itemPoint.x, itemPoint.y );
						return true;
					}
				}
		}

		return false;
	}
}


namespace ui
{
	// MENUITEMINFO_BUFF implementation

	MENUITEMINFO_BUFF::MENUITEMINFO_BUFF( void )
	{
		utl::ZeroWinStruct( static_cast<MENUITEMINFO*>( this ) );
	}

	void MENUITEMINFO_BUFF::ClearTextBuffer( void )
	{
		if ( dwTypeData != nullptr && HasText() )		// text buffer allocated internally?
			delete[] dwTypeData;

		dwTypeData = nullptr;
		cch = 0;
	}

	bool MENUITEMINFO_BUFF::GetMenuItemInfo( HMENU hMenu, UINT item, bool byPos /*= true*/,
											 UINT mask /*= MIIM_ID | MIIM_SUBMENU | MIIM_DATA | MIIM_STATE | MIIM_FTYPE | MIIM_STRING | MIIM_BITMAP*/ )
	{
		ASSERT( ::IsMenu( hMenu ) );

		ClearTextBuffer();
		utl::ZeroWinStruct( static_cast<MENUITEMINFO*>( this ) );

		fMask = mask;					// MIIM_TYPE replaced by MIIM_FTYPE | MIIM_STRING | MIIM_BITMAP

		if ( !::GetMenuItemInfo( hMenu, item, byPos, this ) )
			return false;

		if ( HasFlag( fMask, MIIM_STRING ) && cch != 0 )
		{
			dwTypeData = new TCHAR[ ++cch ];									// enlarge text buffer with EOS & allocate it
			return ::GetMenuItemInfo( hMenu, item, byPos, this ) != FALSE;		// also fetch item text
		}

		return true;
	}
}


namespace dbg
{
	#ifdef _DEBUG

	const CFlagTags& GetTags_MenuItemType( void )
	{
		static const CFlagTags::FlagDef s_flagDefs[] =
		{
			{ FLAG_TAG( MFT_BITMAP ) },
			{ FLAG_TAG( MFT_MENUBARBREAK ) },
			{ FLAG_TAG( MFT_MENUBREAK ) },
			{ FLAG_TAG( MFT_OWNERDRAW ) },
			{ FLAG_TAG( MFT_RADIOCHECK ) },
			{ FLAG_TAG( MFT_SEPARATOR ) },
			{ FLAG_TAG( MFT_RIGHTORDER ) },
			{ FLAG_TAG( MFT_RIGHTJUSTIFY ) }
		};

		static const CFlagTags s_tags( s_flagDefs, COUNT_OF( s_flagDefs ) );
		return s_tags;
	}

	const CFlagTags& GetTags_MenuItemState( void )
	{
		static const CFlagTags::FlagDef s_flagDefs[] =
		{
			{ FLAG_TAG( MF_GRAYED ) },			// MFS_GRAYED is messy: MF_GRAYED | MF_DISABLED
			{ FLAG_TAG( MF_DISABLED ) },		// MFS_DISABLED is messy: MFS_GRAYED
			{ FLAG_TAG( MFS_CHECKED ) },
			{ FLAG_TAG( MFS_HILITE ) },
			{ FLAG_TAG( MFS_DEFAULT ) }
		};

		static const CFlagTags s_tags( s_flagDefs, COUNT_OF( s_flagDefs ) );
		return s_tags;
	}

	std::tstring FormatFlags( const TCHAR fmt[], const CFlagTags& tags, int flags )
	{
		std::tstring coreText = tags.FormatUi( flags );
		if ( coreText.empty() )
			return coreText;			// skip 0 flags

		return str::Format( fmt, coreText.c_str() );
	}

	#endif //_DEBUG
}


namespace dbg
{
	void TrackMenu( CMenu* pPopupMenu, CWnd* pTargetWnd )
	{
	#ifdef _DEBUG
		CPoint point = ui::GetCursorPos();
		pPopupMenu->TrackPopupMenu( TPM_LEFTALIGN | TPM_RETURNCMD, point.x, point.y, pTargetWnd );
	#else
		pPopupMenu, pTargetWnd;
	#endif
	}

	void TraceMenu( HMENU hMenu, unsigned int indentLevel /*= 0*/ )
	{
	#ifdef _DEBUG
		ASSERT( ::IsMenu( hMenu ) );

		for ( int i = 0, count = ::GetMenuItemCount( hMenu ); i != count; ++i )
		{
			ui::MENUITEMINFO_BUFF info;

			if ( info.GetMenuItemInfo( hMenu, i ) )
				TraceMenuItem( info, i, indentLevel );
			else
				ASSERT( false );

			if ( info.hSubMenu != nullptr )
				TraceMenu( info.hSubMenu, indentLevel + 1 );		// trace the sub-menu
		}
	#else
		hMenu, indentLevel;
	#endif
	}

	void TraceMenuItem( HMENU hMenu, int itemPos )
	{
	#ifdef _DEBUG
		ui::MENUITEMINFO_BUFF info;

		if ( info.GetMenuItemInfo( hMenu, itemPos ) )
			TraceMenuItem( info, itemPos );
		else
			TRACE( _T("?? Invalid menu item: hMenu=0x%08x itemPos=%d\n"), hMenu, itemPos );
	#else
		hMenu, itemPos;
	#endif
	}

	void TraceMenuItem( const ui::MENUITEMINFO_BUFF& itemInfo, int itemPos, unsigned int indentLevel /*= 0*/ )
	{
	#ifdef _DEBUG
		static const TCHAR s_space[] = _T(", "), s_fieldSep[] = _T(", "), s_flagsSep[] = _T("   ");
		std::tstring text;

		if ( itemInfo.IsSubMenu() )
			stream::Tag( text, str::Format( _T("hSubMenu=0x%08X"), itemInfo.hSubMenu ), s_space );
		else if ( itemInfo.IsCommand() )
			stream::Tag( text, str::Format( _T("cmdId=%d (0x%X)"), itemInfo.wID, itemInfo.wID ), s_space );

		if ( itemInfo.IsCommand() )
			stream::Tag( text, str::Format( _T("\"%s\""), itemInfo.dwTypeData ), s_fieldSep );		// text

		stream::Tag( text, FormatFlags( _T("Type={%s}"), GetTags_MenuItemType(), itemInfo.fType ), s_flagsSep );				// type flags
		stream::Tag( text, FormatFlags( _T("State={%s}"), GetTags_MenuItemState(), itemInfo.fState ), s_flagsSep );	// state flags

		std::tstring indentPrefix( indentLevel * 2, _T(' ') );
		TRACE( _T("%s[%d] %s\n"), indentPrefix.c_str(), itemPos, text.c_str() );
	#else
		itemInfo, itemPos, indentLevel;
	#endif
	}
}
