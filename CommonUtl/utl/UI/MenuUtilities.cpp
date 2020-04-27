
#include "stdafx.h"
#include "MenuUtilities.h"
#include "ContainerUtilities.h"
#include "ImageStore.h"
#include "Utilities.h"

#ifdef _DEBUG
#include "FlagTags.h"
#define new DEBUG_NEW
#endif


namespace ui
{
	void LoadPopupMenu( CMenu& rContextMenu, UINT menuResId, int popupIndex, UseMenuImages useMenuImages /*= NormalMenuImages*/, std::tstring* pPopupText /*= NULL*/ )
	{
		CMenu menuBar;
		VERIFY( menuBar.LoadMenu( menuResId ) );

		if ( pPopupText != NULL )
		{
			CString popupText;
			menuBar.GetMenuString( popupIndex, popupText, MF_BYPOSITION );
			*pPopupText = popupText.GetString();
		}

		HMENU hSubMenu = ::GetSubMenu( menuBar, popupIndex );
		ASSERT_PTR( hSubMenu );

		rContextMenu.Attach( hSubMenu );
		VERIFY( menuBar.RemoveMenu( popupIndex, MF_BYPOSITION ) );

		if ( useMenuImages != NoMenuImages )
			SetMenuImages( rContextMenu, CheckedMenuImages == useMenuImages );		// set shared bitmap images
	}

	void LoadPopupSubMenu( CMenu& rContextMenu, UINT menuResId, int popupIndex1, int popupIndex2 /*= -1*/, int popupIndex3 /*= -1*/ )
	{
		CMenu menuBar;
		VERIFY( menuBar.LoadMenu( menuResId ) );

		int popupIndex;
		HMENU hParentMenu = menuBar, hSubMenu = safe_ptr( ::GetSubMenu( hParentMenu, popupIndex = popupIndex1 ) );

		if ( popupIndex2 != -1 )
			hSubMenu = safe_ptr( ::GetSubMenu( hParentMenu = hSubMenu, popupIndex = popupIndex2 ) );

		if ( popupIndex3 != -1 )
			hSubMenu = safe_ptr( ::GetSubMenu( hParentMenu = hSubMenu, popupIndex = popupIndex3 ) );

		ASSERT_PTR( hParentMenu );
		ASSERT_PTR( hSubMenu );

		rContextMenu.Attach( hSubMenu );
		VERIFY( ::RemoveMenu( hParentMenu, popupIndex, MF_BYPOSITION ) );		// detach popup from its parent

		SetMenuImages( rContextMenu );											// set shared bitmap images
	}


	bool SetMenuImages( CMenu& rMenu, bool useCheckedBitmaps /*= false*/, CImageStore* pImageStore /*= NULL*/ )
	{
		if ( NULL == pImageStore )
			pImageStore = CImageStore::GetSharedStore();
		if ( NULL == pImageStore )
			return false;

		for ( unsigned int i = 0, count = rMenu.GetMenuItemCount(); i != count; ++i )
		{
			UINT state = rMenu.GetMenuState( i, MF_BYPOSITION );

			if ( HasFlag( state, MF_POPUP ) )
				SetMenuImages( *rMenu.GetSubMenu( i ), useCheckedBitmaps, pImageStore );
			else if ( !HasFlag( state, MF_SEPARATOR | MF_MENUBREAK | MFT_OWNERDRAW ) )
			{
				UINT itemId = rMenu.GetMenuItemID( i );
				ASSERT( itemId != 0 && itemId != UINT_MAX );

				std::pair< CBitmap*, CBitmap* > bitmaps = pImageStore->RetrieveMenuBitmaps( itemId, useCheckedBitmaps );
				if ( bitmaps.first != NULL || bitmaps.second != NULL )
					rMenu.SetMenuItemBitmaps( i, MF_BYPOSITION, bitmaps.first, bitmaps.second );
			}
		}
		return true;
	}

	bool SetMenuItemImage( CMenu& rMenu, const CMenuItemRef& itemRef, UINT iconId /*= 0*/, bool useCheckedBitmaps /*= false*/, CImageStore* pImageStore /*= NULL*/ )
	{
		if ( NULL == pImageStore )
			pImageStore = CImageStore::GetSharedStore();

		if ( pImageStore != NULL )
		{
			std::pair< CBitmap*, CBitmap* > bitmaps = pImageStore->RetrieveMenuBitmaps( 0 == iconId ? itemRef.GetCmdId() : iconId, useCheckedBitmaps );
			if ( bitmaps.first != NULL || bitmaps.second != NULL )
			{
				rMenu.SetMenuItemBitmaps( itemRef.m_itemRef, itemRef.m_refFlags, bitmaps.first, bitmaps.second );
				return true;
			}
		}

		return false;
	}

	int TrackPopupMenu( CMenu& rMenu, CWnd* pTargetWnd, CPoint screenPos, UINT trackFlags /*= TPM_RIGHTBUTTON*/, const RECT* pExcludeRect /*= NULL*/ )
	{
		AdjustMenuTrackPos( screenPos );

		TPMPARAMS excludeParams;
		utl::ZeroWinStruct( &excludeParams );

		if ( pExcludeRect != NULL )			// pExcludeRect is ignored by TrackPopupMenu()
			excludeParams.rcExclude = *pExcludeRect;

		return ui::ToCmdId( rMenu.TrackPopupMenuEx( trackFlags, screenPos.x, screenPos.y, pTargetWnd, pExcludeRect != NULL ? &excludeParams : NULL ) );
	}

	int TrackPopupMenuAlign( CMenu& rMenu, CWnd* pTargetWnd, const RECT& excludeRect, PopupAlign popupAlign /*= DropDown*/,
							 UINT trackFlags /*= TPM_RIGHTBUTTON*/ )
	{
		if ( !HasFlag( trackFlags, TPM_RIGHTALIGN | TPM_BOTTOMALIGN ) )
			trackFlags |= GetAlignTrackFlags( popupAlign );

		return TrackPopupMenu( rMenu, pTargetWnd, GetAlignTrackPos( popupAlign, excludeRect ), trackFlags, &excludeRect );
	}

	CWnd* AutoTargetWnd( CWnd* pTargetWnd )
	{
		CWnd* pCmdTargetWnd = pTargetWnd;

		if ( pTargetWnd != NULL && HasFlag( pTargetWnd->GetStyle(), WS_CHILD ) )
		{	// pTargetWnd is not a dialog
			pCmdTargetWnd = pTargetWnd->GetParentFrame();

			if ( NULL == pCmdTargetWnd )
				pCmdTargetWnd = pTargetWnd->GetParent();

			if ( NULL == pCmdTargetWnd || !pCmdTargetWnd->IsChild( pTargetWnd ) )
				pCmdTargetWnd = pTargetWnd;
		}

		ASSERT_PTR( pCmdTargetWnd );
		return pCmdTargetWnd;
	}

	bool AdjustMenuTrackPos( CPoint& rScreenPos )
	{
		if ( -1 == rScreenPos.x && -1 == rScreenPos.y )
		{
			if ( const CWnd* pFocusWnd = CWnd::GetFocus() )
			{
				CRect targetRect;
				pFocusWnd->GetWindowRect( &targetRect );
				rScreenPos = targetRect.TopLeft();
				rScreenPos += CPoint( 1, 1 );
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



	bool IsValidMenu( HMENU hMenu, unsigned int depth /*= 0*/ )
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
						if ( !IsValidMenu( hSubMenu, depth + 1 ) )
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
	// MENUITEMINFO_BUFF implementation

	MENUITEMINFO_BUFF::MENUITEMINFO_BUFF( void )
	{
		utl::ZeroWinStruct( static_cast< MENUITEMINFO* >( this ) );
	}

	void MENUITEMINFO_BUFF::ClearTextBuffer( void )
	{
		if ( dwTypeData != NULL && HasText() )		// text buffer allocated internally?
			delete[] dwTypeData;

		dwTypeData = NULL;
		cch = 0;
	}

	bool MENUITEMINFO_BUFF::GetMenuItemInfo( HMENU hMenu, UINT item, bool byPos /*= true*/,
											 UINT mask /*= MIIM_ID | MIIM_SUBMENU | MIIM_DATA | MIIM_STATE | MIIM_FTYPE | MIIM_STRING | MIIM_BITMAP*/ )
	{
		ASSERT_PTR( ::IsMenu( hMenu ) );

		ClearTextBuffer();
		utl::ZeroWinStruct( static_cast< MENUITEMINFO* >( this ) );

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


namespace ui
{
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

	UINT GetTotalCmdCount( HMENU hMenu, RecursionDepth depth /*= Deep*/ )
	{
		ASSERT_PTR( hMenu );
		UINT cmdCount = 0;

		for ( UINT i = 0, count = ::GetMenuItemCount( hMenu ); i != count; ++i )
		{
			UINT state = ::GetMenuState( hMenu, i, MF_BYPOSITION );
			ASSERT( state != UINT_MAX );

			if ( HasFlag( state, MF_POPUP ) )
			{
				if ( Deep == depth )
					cmdCount += GetTotalCmdCount( ::GetSubMenu( hMenu, i ), Deep );
			}
			else if ( !HasFlag( state, MF_SEPARATOR ) )
			{
				UINT cmdId = ::GetMenuItemID( hMenu, i );
				if ( cmdId != 0 && cmdId != UINT_MAX )
					++cmdCount;
			}
		}

		return cmdCount;
	}

	void QueryMenuItemIds( std::vector< UINT >& rItemIds, HMENU hMenu )
	{
		ASSERT_PTR( hMenu );

		for ( unsigned int i = 0, count = ::GetMenuItemCount( hMenu ); i != count; ++i )
		{
			UINT state = ::GetMenuState( hMenu, i, MF_BYPOSITION );
			ASSERT( state != UINT_MAX );

			if ( HasFlag( state, MF_POPUP ) )
				QueryMenuItemIds( rItemIds, ::GetSubMenu( hMenu, i ) );
			else if ( HasFlag( state, MFT_OWNERDRAW ) )
				utl::AddUnique( rItemIds, ::GetMenuItemID( hMenu, i ) );		// even separators are added to m_noTouch map
		}
	}

	HMENU CloneMenu( HMENU hSrcMenu )
	{
		ASSERT_PTR( ::IsMenu( hSrcMenu ) );

		CMenu destMenu;
		destMenu.CreatePopupMenu();

		for ( int i = 0, count = ::GetMenuItemCount( hSrcMenu ); i != count; ++i )
		{
			MENUITEMINFO_BUFF itemInfo;
			if ( itemInfo.GetMenuItemInfo( hSrcMenu, i ) )
			{
				if ( itemInfo.hSubMenu != NULL )
					itemInfo.hSubMenu = ui::CloneMenu( itemInfo.hSubMenu );		// clone the sub-menu inplace

				VERIFY( destMenu.InsertMenuItem( i, &itemInfo, TRUE ) );

				if ( !itemInfo.IsSeparator() )
					SetMenuItemImage( destMenu, itemInfo.wID );
			}
			else
				ASSERT( false );		// invalid item?
		}

		return destMenu.Detach();
	}

	size_t CopyMenuItems( CMenu& rDestMenu, unsigned int destIndex, const CMenu& srcMenu, const std::vector< UINT >* pSrcIds /*= NULL*/ )
	{
		size_t copiedCount = 0;

		for ( unsigned int i = 0, srcCount = srcMenu.GetMenuItemCount(); i != srcCount; ++i )
		{
			UINT srcId = srcMenu.GetMenuItemID( i );

			if ( NULL == pSrcIds || ( 0 == srcId || std::find( pSrcIds->begin(), pSrcIds->end(), srcId ) != pSrcIds->end() ) )
			{
				MENUITEMINFO_BUFF itemInfo;
				if ( itemInfo.GetMenuItemInfo( srcMenu, i ) )
				{
					VERIFY( rDestMenu.InsertMenuItem( destIndex, &itemInfo, TRUE ) );

					if ( !itemInfo.IsSeparator() )
						SetMenuItemImage( rDestMenu, itemInfo.wID );

					++copiedCount;
					++destIndex;
				}
				else
					ASSERT( false );
			}
		}

		return copiedCount;
	}

	void DeleteMenuItem( CMenu& rDestMenu, UINT itemId )
	{
		ASSERT( itemId != 0 );

		if ( rDestMenu.DeleteMenu( itemId, MF_BYCOMMAND ) )
			CleanupMenuSeparators( rDestMenu );
	}

	void DeleteMenuItems( CMenu& rDestMenu, const UINT* pItemIds, size_t itemCount )
	{
		ASSERT_PTR( pItemIds );

		for ( unsigned int i = 0; i != itemCount; ++i )
		{
			ASSERT( pItemIds[ i ] != 0 );
			rDestMenu.DeleteMenu( pItemIds[ i ], MF_BYCOMMAND );
		}

		CleanupMenuSeparators( rDestMenu );
	}

	void CleanupMenuDuplicates( CMenu& rDestMenu )
	{
		// first delete any duplicate commands (no separators)
		for ( int i = 0, count = rDestMenu.GetMenuItemCount(); i != count; ++i )
		{
			UINT itemId = rDestMenu.GetMenuItemID( i );

			if ( UINT_MAX == itemId ) // sub-menu
			{
				if ( CMenu* pSubMenu = rDestMenu.GetSubMenu( i ) )
					CleanupMenuDuplicates( *pSubMenu );
			}
			else if ( itemId != 0 ) // normal command, no separator
			{
				for ( ;; )
				{
					unsigned int iDuplicate = FindMenuItemIndex( rDestMenu, itemId, i + 1 );

					if ( iDuplicate != UINT_MAX ) // found a duplicate
						rDestMenu.DeleteMenu( iDuplicate, MF_BYPOSITION ); // delete duplicate command
					else
						break;
				}
			}
		}

		CleanupMenuSeparators( rDestMenu );
	}

	void CleanupMenuSeparators( CMenu& rDestMenu )
	{
		UINT i = rDestMenu.GetMenuItemCount();
		for ( UINT prevId = UINT_MAX; i-- != 0; )
		{
			UINT itemId = rDestMenu.GetMenuItemID( i );

			if ( 0 == itemId && 0 == prevId )
				rDestMenu.DeleteMenu( i, MF_BYPOSITION );		// delete double separator
			else
				prevId = itemId;
		}

		DeleteFirstMenuSeparator( rDestMenu );
		DeleteLastMenuSeparator( rDestMenu );
	}

	bool DeleteFirstMenuSeparator( CMenu& rDestMenu )
	{
		return
			rDestMenu.GetMenuItemCount() != 0 &&
			0 == rDestMenu.GetMenuItemID( 0 ) &&
			rDestMenu.DeleteMenu( 0, MF_BYPOSITION ) != FALSE;		// delete first separator
	}

	bool DeleteLastMenuSeparator( CMenu& rDestMenu )
	{
		UINT i = rDestMenu.GetMenuItemCount();

		return
			i != 0 &&
			0 == rDestMenu.GetMenuItemID( --i ) &&
			rDestMenu.DeleteMenu( i, MF_BYPOSITION ) != FALSE;		// delete last separator
	}


	bool JoinMenuItems( CMenu& rDestMenu, const CMenu& srcMenu, MenuInsert menuInsert /*= AppendSrc*/, bool addSep /*= true*/, UseMenuImages useMenuImages /*= NormalMenuImages*/ )
	{
		ASSERT_PTR( rDestMenu.GetSafeHmenu() );
		ASSERT_PTR( srcMenu.GetSafeHmenu() );

		if ( 0 == srcMenu.GetMenuItemCount() )
			return false;

		switch ( menuInsert )
		{
			case PrependSrc:
				if ( addSep )
					rDestMenu.InsertMenu( 0, MF_SEPARATOR | MF_BYPOSITION );
				CopyMenuItems( rDestMenu, 0, srcMenu );
				break;
			case AppendSrc:
				if ( addSep )
					rDestMenu.InsertMenu( rDestMenu.GetMenuItemCount(), MF_SEPARATOR | MF_BYPOSITION );
				CopyMenuItems( rDestMenu, rDestMenu.GetMenuItemCount(), srcMenu );
				break;
		}

		CleanupMenuDuplicates( rDestMenu );
		CleanupMenuSeparators( rDestMenu );

		if ( useMenuImages != NoMenuImages )
			SetMenuImages( rDestMenu, CheckedMenuImages == useMenuImages );

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

		return NULL;
	}

	bool IsHiliteMenuItem( HMENU hMenu, int itemPos )
	{
		ASSERT_PTR( hMenu );

		UINT itemState = ::GetMenuState( hMenu, itemPos, MF_BYPOSITION );
		if ( itemState != UINT_MAX )
			if ( !HasFlag( itemState, MF_GRAYED | MF_DISABLED | MF_POPUP | MF_SEPARATOR ) )
				return HasFlag( itemState, MF_HILITE );

		return false;
	}

	void InvalidateMenuWindow( void )
	{
		if ( HWND hMenuWnd = FindMenuWindowFromPoint() )
			::InvalidateRect( hMenuWnd, NULL, TRUE );
	}


	bool ScrollVisibleMenuItem( HWND hTrackWnd, HMENU hMenu, UINT hiliteId )
	{
		ASSERT( hiliteId != 0 );
		ASSERT_PTR( hTrackWnd );
		ASSERT_PTR( ::IsMenu( hMenu ) );

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
		ASSERT_PTR( ::IsMenu( hMenu ) );

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
	void TraceMenu( HMENU hMenu, unsigned int indentLevel /*= 0*/ )
	{
	#ifdef _DEBUG
		ASSERT_PTR( ::IsMenu( hMenu ) );

		for ( int i = 0, count = ::GetMenuItemCount( hMenu ); i != count; ++i )
		{
			ui::MENUITEMINFO_BUFF itemInfo;

			if ( itemInfo.GetMenuItemInfo( hMenu, i ) )
				TraceMenuItem( itemInfo, i, indentLevel );
			else
				ASSERT( false );

			if ( itemInfo.hSubMenu != NULL )
				TraceMenu( itemInfo.hSubMenu, indentLevel + 1 );		// trace the sub-menu
		}
	#else
		hMenu, indentLevel;
	#endif
	}

	void TraceMenuItem( HMENU hMenu, int itemPos )
	{
	#ifdef _DEBUG
		ui::MENUITEMINFO_BUFF itemInfo;

		if ( itemInfo.GetMenuItemInfo( hMenu, itemPos ) )
			TraceMenuItem( itemInfo, itemPos );
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
