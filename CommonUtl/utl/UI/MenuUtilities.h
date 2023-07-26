#ifndef MenuUtilities_h
#define MenuUtilities_h
#pragma once

#include "ui_fwd.h"
#include <afxcontextmenumanager.h>		// for afxContextMenuManager


namespace ui
{
	interface IImageStore;
	class CPopupIndexPath;		// store a popup index depth path of up to 4 depth levels; defined further down
	struct CMenuItemRef;		// defined further down


	enum UseMenuImages { NoMenuImages, NormalMenuImages, CheckedMenuImages };


	bool LoadPopupMenu( OUT CMenu* pContextMenu, UINT menuResId, const CPopupIndexPath& popupIndexPath, UseMenuImages useMenuImages = NormalMenuImages, OUT std::tstring* pOutPopupText = nullptr );

	bool SetMenuImages( OUT CMenu* pMenu, bool useCheckedBitmaps = false, ui::IImageStore* pImageStore = nullptr );
	bool SetMenuItemImage( OUT CMenu* pMenu, const CMenuItemRef& itemRef, UINT iconId = 0, bool useCheckedBitmaps = false, ui::IImageStore* pImageStore = nullptr );

	int TrackPopupMenu( CMenu& rMenu, CWnd* pTargetWnd, CPoint screenPos, UINT trackFlags = TPM_RIGHTBUTTON, const RECT* pExcludeRect = nullptr );
	int TrackPopupMenuAlign( CMenu& rMenu, CWnd* pTargetWnd, const RECT& excludeRect, PopupAlign popupAlign = DropDown, UINT trackFlags = TPM_RIGHTBUTTON );


	// context menu loading and tracking

	int TrackContextMenu( UINT menuResId, const CPopupIndexPath& popupIndexPath, CWnd* pTargetWnd, CPoint screenPos, UINT trackFlags = TPM_RIGHTBUTTON, const RECT* pExcludeRect = nullptr );


	// new MFC style popup menus:

	inline bool UseMfcMenuManager( void ) { return afxContextMenuManager != NULL; }

	inline bool LoadMfcPopupMenu( CMenu* pContextMenu, UINT menuResId, const CPopupIndexPath& popupIndexPath, OUT std::tstring* pOutPopupText = nullptr )
	{
		return LoadPopupMenu( pContextMenu, menuResId, popupIndexPath, UseMfcMenuManager() ? NoMenuImages : NormalMenuImages, pOutPopupText );		// auto-images
	}


	int TrackMfcPopupMenu( HMENU hPopupMenu, CWnd* pTargetWnd, CPoint screenPos, bool sendCommand = true );		// track modal: always returns the command ID

	int TrackMfcContextMenu( UINT menuResId, const CPopupIndexPath& popupIndexPath, CWnd* pTargetWnd, CPoint screenPos, bool sendCommand = true );		// track modal: always returns the command ID


	CWnd* AutoTargetWnd( CWnd* pTargetWnd );
	bool AdjustMenuTrackPos( CPoint& rScreenPos );
	DWORD GetAlignTrackFlags( PopupAlign popupAlign );
	CPoint GetAlignTrackPos( PopupAlign popupAlign, const RECT& excludeRect );
}


namespace ui
{
	inline CMenu* SafeFromHandle( HMENU hMenu )		// safe version, for convenient conditional expressions
	{
		REQUIRE( nullptr == hMenu || ::IsMenu( hMenu ) );
		return hMenu != nullptr ? CMenu::FromHandle( hMenu ) : nullptr;
	}


	// menu item

	inline std::tstring GetMenuItemText( const CMenu* pMenu, UINT item, UINT flags = MF_BYCOMMAND )
	{
		CString itemText;
		pMenu->GetMenuString( item, itemText, flags );
		return itemText.GetString();
	}


	UINT GetMenuItemType( HMENU hMenu, UINT item, bool byPos = true );						// 'item' is either itemId or itemIndex, depending on byPos

	void* GetMenuItemData( HMENU hMenu, UINT item, bool byPos = true );						// 'item' is either itemId or itemIndex, depending on byPos
	bool SetMenuItemData( HMENU hMenu, UINT item, const void* pItemData, bool byPos = true );

	template< typename Type >
	inline Type* GetMenuItemPtr( HMENU hMenu, UINT item, bool byPos = true ) { return reinterpret_cast<Type*>( GetMenuItemData( hMenu, item, byPos ) ); }

	template< typename Type >
	inline bool SetMenuItemPtr( HMENU hMenu, UINT item, const Type* pItemPtr, bool byPos = true ) { return SetMenuItemData( hMenu, item, pItemPtr, byPos ); }

	inline bool HasSeparatorItemState( UINT itemState ) { return itemState != UINT_MAX && HasFlag( itemState, MF_SEPARATOR | MF_MENUBARBREAK | MF_MENUBREAK ); }
	inline bool IsSeparatorItem( HMENU hMenu, UINT item, bool byPos = true ) { return HasSeparatorItemState( GetMenuItemType( hMenu, item, byPos ) ); }


	// shallow menu API

	int FindMenuItemIndex( HMENU hMenu, UINT itemId, unsigned int iFirst = 0 );
	int FindAfterMenuItemIndex( HMENU hMenu, UINT itemId, unsigned int iFirst = 0 );		// subsequent position

	// deep menu API

	HMENU FindMenuItemIndex( OUT int* pOutIndex, HMENU hMenu, UINT itemId, RecursionDepth depth = Deep );
	inline CMenu* FindMenuItemIndex( OUT int* pOutIndex, const CMenu* pMenu, UINT itemId, RecursionDepth depth = Deep )
	{
		return ui::SafeFromHandle( FindMenuItemIndex( pOutIndex, pMenu->GetSafeHmenu(), itemId, depth ) );
	}

	HMENU FindFirstMenuCommand( OUT UINT* pOutCmdId, HMENU hMenu, RecursionDepth depth = Deep );		// first valid command (not separator)

	UINT GetTotalCmdCount( HMENU hMenu, RecursionDepth depth = Deep );						// just commands (excluding separators, sub-menus)
	void QueryMenuItemIds( std::vector<UINT>& rItemIds, HMENU hMenu, RecursionDepth depth = Deep );

	HMENU CloneMenu( HMENU hSrcMenu );

	size_t CopyMenuItems( OUT CMenu* pDestMenu, unsigned int destIndex, const CMenu* pSrcMenu, const std::vector<UINT>* pSrcIds = nullptr );
	void DeleteMenuItem( OUT CMenu* pDestMenu, UINT itemId );
	size_t DeleteMenuItems( OUT CMenu* pDestMenu, const UINT* pItemIds, size_t itemCount );

	size_t CleanupMenuDuplicates( OUT CMenu* pDestMenu );
	size_t CleanupMenuSeparators( OUT CMenu* pDestMenu );

	enum MenuInsert { PrependSrc, AppendSrc };

	bool JoinMenuItems( OUT CMenu* pDestMenu, const CMenu* pSrcMenu, MenuInsert menuInsert = AppendSrc, bool addSep = true, UseMenuImages useMenuImages = NormalMenuImages );


	bool EnsureDeepValidMenu( HMENU hMenu, unsigned int depth = 0 );
}


namespace ui
{
	// menu hit-test
	HWND FindMenuWindowFromPoint( CPoint screenPos = ui::GetCursorPos() );
	bool IsHiliteMenuItem( HMENU hMenu, int itemPos );
	void InvalidateMenuWindow( void );

	bool ScrollVisibleMenuItem( HWND hTrackWnd, HMENU hMenu, UINT hiliteId );
	bool HoverOnMenuItem( HWND hTrackWnd, HMENU hMenu, UINT hiliteId );			// don't call this directly (menu must be visible): call delayed through ui::PostCall when handling WM_INITMENUPOPUP
}


namespace ui
{
	class CPopupIndexPath		// store a popup index depth path of up to 4 depth levels
	{
	public:
		CPopupIndexPath( int popupIndex0, int popupIndex1 = -1, int popupIndex2 = -1, int popupIndex3 = -1 )
		{
			m_popups[ 0 ] = static_cast<signed char>( popupIndex0 );
			m_popups[ 1 ] = static_cast<signed char>( popupIndex1 );
			m_popups[ 2 ] = static_cast<signed char>( popupIndex2 );
			m_popups[ 3 ] = static_cast<signed char>( popupIndex3 );

			m_depth = 0;
			while ( m_depth != DepthLimit && m_popups[ m_depth ] != -1 )
				++m_depth;
		}

		size_t GetDepth( void ) const { return m_depth; }
		int GetPopupIndexAt( size_t depth ) const { ASSERT( depth < m_depth ); return static_cast<int>( m_popups[ depth ] ); }
	private:
		enum { DepthLimit = 4 };

		signed char m_popups[ DepthLimit ];		// popup indexes by depth level [0, 3]
		size_t m_depth;
	};


	struct CMenuItemRef
	{
		CMenuItemRef( UINT itemRef, UINT refFlags = MF_BYCOMMAND )			// item qualified with ID by default
			: m_itemRef( itemRef ), m_refFlags( refFlags ) {}

		static CMenuItemRef ByPosition( UINT itemPos ) { return CMenuItemRef( itemPos, MF_BYPOSITION ); }

		bool IsByPosition( void ) const { return HasFlag( m_refFlags, MF_BYPOSITION ); }
		bool IsByCommand( void ) const { return !IsByPosition(); }
		UINT GetCmdId( void ) const { ASSERT( IsByCommand() ); return m_itemRef; }
	public:
		UINT m_itemRef;
		UINT m_refFlags;
	};


	// with managed memory for the text buffer
	struct MENUITEMINFO_BUFF : public MENUITEMINFO
							 , private utl::noncopyable
	{
		MENUITEMINFO_BUFF( void );
		~MENUITEMINFO_BUFF() { ClearTextBuffer(); }

		void ClearTextBuffer( void );

		bool GetMenuItemInfo( HMENU hMenu, UINT item, bool byPos = true,
							  UINT mask = MIIM_ID | MIIM_SUBMENU | MIIM_DATA | MIIM_STATE | MIIM_FTYPE | MIIM_STRING | MIIM_BITMAP );

		bool IsSeparator( void ) const { return HasFlag( fType, MFT_SEPARATOR ); }
		bool IsSubMenu( void ) const { return hSubMenu != nullptr; }
		bool IsCommand( void ) const { return !IsSeparator() && !IsSubMenu() && wID != 0; }

		enum { BaseTypeMask = MFT_STRING | MFT_BITMAP | MFT_SEPARATOR };

		bool HasText( void ) const { return EqMaskedValue( fType, BaseTypeMask, MFT_STRING ); }
	};
}


namespace dbg
{
	void TrackMenu( CMenu* pPopupMenu, CWnd* pTargetWnd );

	void TraceMenu( HMENU hMenu, unsigned int indentLevel = 0 );
	void TraceMenuItem( HMENU hMenu, int itemPos );
	void TraceMenuItem( const ui::MENUITEMINFO_BUFF& itemInfo, int itemPos, unsigned int indentLevel = 0 );
}


#endif // MenuUtilities_h
