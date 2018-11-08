
#include "stdafx.h"
#include "MenuUtilities.h"
#include "ContainerUtilities.h"
#include "ImageStore.h"
#include "VersionInfo.h"

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

	bool SetMenuItemImage( CMenu& rMenu, UINT itemId, UINT iconId /*= 0*/, bool useCheckedBitmaps /*= false*/, CImageStore* pImageStore /*= NULL*/ )
	{
		if ( NULL == pImageStore )
			pImageStore = CImageStore::GetSharedStore();

		if ( pImageStore != NULL )
		{
			std::pair< CBitmap*, CBitmap* > bitmaps = pImageStore->RetrieveMenuBitmaps( 0 == iconId ? itemId : iconId, useCheckedBitmaps );
			if ( bitmaps.first != NULL || bitmaps.second != NULL )
			{
				rMenu.SetMenuItemBitmaps( itemId, MF_BYCOMMAND, bitmaps.first, bitmaps.second );
				return true;
			}
		}

		return false;
	}

	int TrackPopupMenu( CMenu& rMenu, CWnd* pTargetWnd, CPoint screenPos, UINT trackFlags /*= TPM_RIGHTBUTTON*/, const RECT* pExcludeRect /*= NULL*/ )
	{
		TPMPARAMS excludeStruct = { sizeof( TPMPARAMS ) };

		if ( pExcludeRect != NULL ) // pExcludeRect is ignored by TrackPopupMenu()
			excludeStruct.rcExclude = *pExcludeRect;

		AdjustMenuTrackPos( screenPos );
		return rMenu.TrackPopupMenuEx( trackFlags, screenPos.x, screenPos.y, pTargetWnd, pExcludeRect != NULL ? &excludeStruct : NULL );
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


	bool GetMenuItemInfo( MENUITEMINFO* pItemInfo, HMENU hMenu, UINT item, bool byPos /*= true*/,
						  UINT mask /*= MIIM_ID | MIIM_SUBMENU | MIIM_DATA | MIIM_STATE | MIIM_FTYPE | MIIM_STRING | MIIM_BITMAP*/ )
	{
		ASSERT_PTR( pItemInfo );
		ASSERT_PTR( ::IsMenu( hMenu ) );

		static TCHAR s_itemText[ 512 ];
		utl::ZeroWinStruct( pItemInfo );
		s_itemText[ 0 ] = _T('\0');

		pItemInfo->fMask = mask;					// MIIM_TYPE replaced by MIIM_FTYPE | MIIM_STRING | MIIM_BITMAP
		pItemInfo->dwTypeData = s_itemText;
		pItemInfo->cch = COUNT_OF( s_itemText );

		return ::GetMenuItemInfo( hMenu, item, byPos, pItemInfo ) != FALSE;
	}


	unsigned int FindMenuItemIndex( const CMenu& rMenu, UINT itemId, unsigned int iFirst /*= 0*/ )
	{
		for ( unsigned int count = rMenu.GetMenuItemCount(); iFirst < count; ++iFirst )
			if ( rMenu.GetMenuItemID( iFirst ) == itemId )
				return iFirst;

		return UINT_MAX;
	}

	unsigned int FindAfterMenuItemIndex( const CMenu& rMenu, UINT itemId, unsigned int iFirst /*= 0*/ )
	{
		unsigned int atIndex = FindMenuItemIndex( rMenu, itemId, iFirst );
		if ( UINT_MAX == atIndex )
			return atIndex;
		return atIndex + 1; // the position just after
	}

	void QueryMenuItemIds( std::vector< UINT >& rItemIds, const CMenu& rMenu )
	{
		for ( unsigned int i = 0, count = rMenu.GetMenuItemCount(); i != count; ++i )
		{
			UINT state = rMenu.GetMenuState( i, MF_BYPOSITION );

			if ( HasFlag( state, MF_POPUP ) )
				QueryMenuItemIds( rItemIds, *rMenu.GetSubMenu( i ) );
			else if ( HasFlag( state, MFT_OWNERDRAW ) )
				utl::AddUnique( rItemIds, rMenu.GetMenuItemID( i ) ); // even separators are added to m_noTouch map
		}
	}

	HMENU CloneMenu( HMENU hSrcMenu )
	{
		ASSERT_PTR( ::IsMenu( hSrcMenu ) );

		HMENU hDestMenu = ::CreatePopupMenu();

		for ( int i = 0, count = ::GetMenuItemCount( hSrcMenu ); i != count; ++i )
		{
			MENUITEMINFO itemInfo;
			if ( ui::GetMenuItemInfo( &itemInfo, hSrcMenu, i ) )
			{
				if ( itemInfo.hSubMenu != NULL )
					itemInfo.hSubMenu = ui::CloneMenu( itemInfo.hSubMenu ); // clone the sub-menu

				VERIFY( ::InsertMenuItem( hDestMenu, i, TRUE, &itemInfo ) );
			}
			else
				ASSERT( false );		// invalid item?
		}

		return hDestMenu;
	}

	size_t CopyMenuItems( CMenu& rDestMenu, unsigned int destIndex, const CMenu& srcMenu, const std::vector< UINT >* pSrcIds /*= NULL*/ )
	{
		size_t copiedCount = 0;

		for ( unsigned int i = 0, srcCount = srcMenu.GetMenuItemCount(); i != srcCount; ++i )
		{
			UINT srcId = srcMenu.GetMenuItemID( i );

			if ( NULL == pSrcIds || ( 0 == srcId || std::find( pSrcIds->begin(), pSrcIds->end(), srcId ) != pSrcIds->end() ) )
			{
				MENUITEMINFO itemInfo;
				if ( ui::GetMenuItemInfo( &itemInfo, srcMenu, i ) )
				{
					VERIFY( rDestMenu.InsertMenuItem( destIndex, &itemInfo, TRUE ) );

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
				rDestMenu.DeleteMenu( i, MF_BYPOSITION ); // delete double separator
			else
				prevId = itemId;
		}

		if ( rDestMenu.GetMenuItemCount() != 0 && 0 == rDestMenu.GetMenuItemID( 0 ) )
			rDestMenu.DeleteMenu( 0, MF_BYPOSITION ); // delete first separator

		if ( UINT i = rDestMenu.GetMenuItemCount() )
			if ( 0 == rDestMenu.GetMenuItemID( --i ) )
				rDestMenu.DeleteMenu( i, MF_BYPOSITION ); // delete last separator
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
			state.m_pParentMenu = pPopupMenu;	// parent == child for tracking popup
		else if ( ( hParentMenu = ::GetMenu( pWindow->m_hWnd ) ) != NULL )
		{
			CWnd* pParent = pWindow->GetTopLevelParent();
				// child windows don't have menus -- need to go to the top!
			if ( pParent != NULL &&
				( hParentMenu = ::GetMenu( pParent->m_hWnd ) ) != NULL )
			{
				int nIndexMax = ::GetMenuItemCount( hParentMenu );
				for ( int nIndex = 0; nIndex < nIndexMax; nIndex++ )
				{
					if ( ::GetSubMenu( hParentMenu, nIndex ) == pPopupMenu->m_hMenu )
					{
						// when popup is found, m_pParentMenu is containing menu
						state.m_pParentMenu = CMenu::FromHandle( hParentMenu );
						break;
					}
				}
			}
		}

		state.m_nIndexMax = pPopupMenu->GetMenuItemCount();
		for ( state.m_nIndex = 0; state.m_nIndex < state.m_nIndexMax; ++state.m_nIndex )
		{
			state.m_nID = pPopupMenu->GetMenuItemID( state.m_nIndex );
			if ( state.m_nID == 0 )
				continue; // menu separator or invalid cmd - ignore it

			ASSERT( state.m_pOther == NULL );
			ASSERT( state.m_pMenu != NULL );
			if ( state.m_nID == (UINT)-1 )
			{
				// possibly a popup menu, route to first item of that popup
				state.m_pSubMenu = pPopupMenu->GetSubMenu( state.m_nIndex );
				if ( state.m_pSubMenu == NULL ||
					( state.m_nID = state.m_pSubMenu->GetMenuItemID( 0 ) ) == 0 || state.m_nID == (UINT)-1 )
					continue;	   // first item of popup can't be routed to

				state.DoUpdate( pWindow, FALSE ); // popups are never auto disabled
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

} // namespace ui


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
			{ FLAG_TAG( MFS_ENABLED ) },
			{ FLAG_TAG( MFS_UNCHECKED ) },
			{ FLAG_TAG( MFS_UNHILITE ) },
			{ FLAG_TAG( MFS_DEFAULT ) }
		};

		static const CFlagTags s_tags( s_flagDefs, COUNT_OF( s_flagDefs ) );
		return s_tags;
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
			MENUITEMINFO itemInfo;

			if ( ui::GetMenuItemInfo( &itemInfo, hMenu, i ) )
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

	void TraceMenuItem( const MENUITEMINFO& itemInfo, int itemPos, unsigned int indentLevel /*= 0*/ )
	{
	#ifdef _DEBUG
		static const TCHAR s_space[] = _T(", "), s_fieldSep[] = _T(", "), s_flagsSep[] = _T("   "), s_tagSep[] = _T("|");
		std::tstring text;

		if ( itemInfo.hSubMenu != NULL )
			stream::Tag( text, str::Format( _T("hSubMenu=0x%08X"), itemInfo.hSubMenu ), s_space );
		else
			stream::Tag( text, str::Format( _T("cmdId=%d=0x%X"), itemInfo.wID, itemInfo.wID ), s_space );

		stream::Tag( text, str::Format( _T("\"%s\""), itemInfo.dwTypeData ), s_fieldSep );		// text

		stream::Tag( text, str::Format( _T("Type={%s}"), GetTags_MenuItemType().FormatUi( itemInfo.fType, s_tagSep ) ), s_flagsSep );		// type flags
		stream::Tag( text, str::Format( _T("State={%s}"), GetTags_MenuItemState().FormatUi( itemInfo.fState, s_tagSep ) ), s_flagsSep );	// state flags

		std::tstring indentPrefix( indentLevel * 2, _T(' ') );
		TRACE( _T("%s[%d] %s\n"), indentPrefix.c_str(), itemPos, text.c_str() );
	#else
		itemInfo, itemPos, indentLevel;
	#endif
	}
}
