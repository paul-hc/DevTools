#ifndef MenuUtilities_h
#define MenuUtilities_h
#pragma once

#include "ui_fwd.h"
#include <afxcontextmenumanager.h>


namespace ui
{
	interface IImageStore;
	struct CMenuItemRef;			// defined further down


	enum DeepPopupIndex {};			// composite of nested popup indexes (maximum 4 levels deep) - empty enum defines a type that has resolution for overloading

	inline DeepPopupIndex MakeDeepPopupIndex( int popupIndex0, int popupIndex1 = -1, int popupIndex2 = -1, int popupIndex3 = -1 )
	{
		return static_cast<DeepPopupIndex>( popupIndex0 | ( popupIndex1 << 8 ) | ( popupIndex2 << 16 ) | ( popupIndex3 << 24 ) );
	}

	inline int GetPopupIndexAt( DeepPopupIndex deepPopupIndex, unsigned int depthLevel )
	{
		REQUIRE( depthLevel < 4 );
		enum { FieldMask = 0xFF };

		return (int)(signed char)( ( static_cast<int>( deepPopupIndex ) >> (depthLevel * 8) ) & FieldMask );
	}


	enum UseMenuImages { NoMenuImages, NormalMenuImages, CheckedMenuImages };


	bool LoadPopupMenu( CMenu* pContextMenu, UINT menuResId, DeepPopupIndex deepPopupIndex, UseMenuImages useMenuImages = NormalMenuImages, OUT std::tstring* pOutPopupText = nullptr );

	inline
	bool LoadPopupMenu( CMenu* pContextMenu, UINT menuResId, int popupIndex, UseMenuImages useMenuImages = NormalMenuImages, OUT std::tstring* pOutPopupText = nullptr )
	{
		return LoadPopupMenu( pContextMenu, menuResId, MakeDeepPopupIndex( popupIndex ), useMenuImages, pOutPopupText );
	}

	bool SetMenuImages( CMenu& rMenu, bool useCheckedBitmaps = false, ui::IImageStore* pImageStore = nullptr );
	bool SetMenuItemImage( CMenu& rMenu, const CMenuItemRef& itemRef, UINT iconId = 0, bool useCheckedBitmaps = false, ui::IImageStore* pImageStore = nullptr );

	int TrackPopupMenu( CMenu& rMenu, CWnd* pTargetWnd, CPoint screenPos, UINT trackFlags = TPM_RIGHTBUTTON, const RECT* pExcludeRect = nullptr );
	int TrackPopupMenuAlign( CMenu& rMenu, CWnd* pTargetWnd, const RECT& excludeRect, PopupAlign popupAlign = DropDown, UINT trackFlags = TPM_RIGHTBUTTON );


	// context menu loading and tracking

	int TrackContextMenu( UINT menuResId, DeepPopupIndex deepPopupIndex, CWnd* pTargetWnd, CPoint screenPos, UINT trackFlags = TPM_RIGHTBUTTON, const RECT* pExcludeRect = nullptr );

	inline
	int TrackContextMenu( UINT menuResId, int popupIndex, CWnd* pTargetWnd, CPoint screenPos, UINT trackFlags = TPM_RIGHTBUTTON, const RECT* pExcludeRect = nullptr )
	{
		return TrackContextMenu( menuResId, MakeDeepPopupIndex( popupIndex ), pTargetWnd, screenPos, trackFlags, pExcludeRect );
	}


	// new MFC style popup menus:

	inline bool UseMfcMenuManager( void ) { return afxContextMenuManager != NULL; }

	inline bool LoadMfcPopupMenu( CMenu* pContextMenu, UINT menuResId, DeepPopupIndex deepPopupIndex, OUT std::tstring* pOutPopupText = nullptr )
	{
		return LoadPopupMenu( pContextMenu, menuResId, deepPopupIndex, UseMfcMenuManager() ? NoMenuImages : NormalMenuImages, pOutPopupText );		// auto-images
	}

	inline bool LoadMfcPopupMenu( CMenu* pContextMenu, UINT menuResId, int popupIndex, OUT std::tstring* pOutPopupText = nullptr )
	{
		return LoadPopupMenu( pContextMenu, menuResId, popupIndex, UseMfcMenuManager() ? NoMenuImages : NormalMenuImages, pOutPopupText );			// auto-images
	}


	int TrackMfcPopupMenu( HMENU hPopupMenu, CWnd* pTargetWnd, CPoint screenPos, bool sendCommand = true );						// always returns the command ID

	int TrackMfcContextMenu( UINT menuResId, DeepPopupIndex deepPopupIndex, CWnd* pTargetWnd, CPoint screenPos, bool sendCommand = true );		// always returns the command ID

	inline
	int TrackMfcContextMenu( UINT menuResId, int popupIndex, CWnd* pTargetWnd, CPoint screenPos, bool sendCommand = true )		// always returns the command ID
	{
		return TrackMfcContextMenu( menuResId, MakeDeepPopupIndex( popupIndex ), pTargetWnd, screenPos, sendCommand );
	}


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

	inline std::tstring GetMenuItemText( const CMenu& menu, UINT itemId, UINT flags = MF_BYCOMMAND )
	{
		CString itemText;
		menu.GetMenuString( itemId, itemText, flags );
		return itemText.GetString();
	}

	// shallow menu API

	int FindMenuItemIndex( HMENU hMenu, UINT itemId, unsigned int iFirst = 0 );
	int FindAfterMenuItemIndex( HMENU hMenu, UINT itemId, unsigned int iFirst = 0 );		// subsequent position

	// deep menu API

	HMENU FindMenuItemIndex( OUT int* pOutIndex, HMENU hMenu, UINT itemId, RecursionDepth depth = Deep );
	HMENU FindFirstMenuCommand( OUT UINT* pOutCmdId, HMENU hMenu, RecursionDepth depth = Deep );		// first valid command (not separator)

	UINT GetTotalCmdCount( HMENU hMenu, RecursionDepth depth = Deep );						// just commands (excluding separators, sub-menus)
	void QueryMenuItemIds( std::vector<UINT>& rItemIds, HMENU hMenu, RecursionDepth depth = Deep );

	HMENU CloneMenu( HMENU hSrcMenu );

	size_t CopyMenuItems( CMenu& rDestMenu, unsigned int destIndex, const CMenu& srcMenu, const std::vector<UINT>* pSrcIds = nullptr );
	void DeleteMenuItem( CMenu& rDestMenu, UINT itemId );
	void DeleteMenuItems( CMenu& rDestMenu, const UINT* pItemIds, size_t itemCount );

	void CleanupMenuDuplicates( CMenu& rDestMenu );
	void CleanupMenuSeparators( CMenu& rDestMenu );

	bool DeleteFirstMenuSeparator( CMenu& rDestMenu );
	bool DeleteLastMenuSeparator( CMenu& rDestMenu );

	enum MenuInsert { PrependSrc, AppendSrc };

	bool JoinMenuItems( CMenu& rDestMenu, const CMenu& srcMenu, MenuInsert menuInsert = AppendSrc, bool addSep = true, UseMenuImages useMenuImages = NormalMenuImages );


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
	void TraceMenu( HMENU hMenu, unsigned int indentLevel = 0 );
	void TraceMenuItem( HMENU hMenu, int itemPos );
	void TraceMenuItem( const ui::MENUITEMINFO_BUFF& itemInfo, int itemPos, unsigned int indentLevel = 0 );
}


#endif // MenuUtilities_h
