#ifndef MenuUtilities_h
#define MenuUtilities_h
#pragma once

#include "ui_fwd.h"


class CImageStore;


// imports from <afximpl.h>
void AFXAPI AfxCancelModes( HWND hWndRcvr );


namespace ui
{
	enum UseMenuImages { NoMenuImages, NormalMenuImages, CheckedMenuImages };


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


	void LoadPopupMenu( CMenu& rContextMenu, UINT menuResId, int popupIndex, UseMenuImages useMenuImages = NormalMenuImages, std::tstring* pPopupText = NULL );
	void LoadPopupSubMenu( CMenu& rContextMenu, UINT menuResId, int popupIndex1, int popupIndex2 = -1, int popupIndex3 = -1 );

	bool SetMenuImages( CMenu& rMenu, bool useCheckedBitmaps = false, CImageStore* pImageStore = NULL );
	bool SetMenuItemImage( CMenu& rMenu, const CMenuItemRef& itemRef, UINT iconId = 0, bool useCheckedBitmaps = false, CImageStore* pImageStore = NULL );

	int TrackPopupMenu( CMenu& rMenu, CWnd* pTargetWnd, CPoint screenPos, UINT trackFlags = TPM_RIGHTBUTTON, const RECT* pExcludeRect = NULL );
	int TrackPopupMenuAlign( CMenu& rMenu, CWnd* pTargetWnd, const RECT& excludeRect, PopupAlign popupAlign = DropDown, UINT trackFlags = TPM_RIGHTBUTTON );


	CWnd* AutoTargetWnd( CWnd* pTargetWnd );
	bool AdjustMenuTrackPos( CPoint& rScreenPos );
	DWORD GetAlignTrackFlags( PopupAlign popupAlign );
	CPoint GetAlignTrackPos( PopupAlign popupAlign, const RECT& excludeRect );


	// menu item
	bool GetMenuItemInfo( MENUITEMINFO* pItemInfo, HMENU hMenu, UINT item, bool byPos = true,
						  UINT mask = MIIM_ID | MIIM_SUBMENU | MIIM_DATA | MIIM_STATE | MIIM_FTYPE | MIIM_STRING | MIIM_BITMAP );

	inline bool IsSeparatorItem( const MENUITEMINFO& itemInfo ) { return HasFlag( itemInfo.fType, MFT_SEPARATOR ); }
	inline bool IsSubMenuItem( const MENUITEMINFO& itemInfo ) { return itemInfo.hSubMenu != NULL; }
	inline bool IsCommandItem( const MENUITEMINFO& itemInfo ) { return !IsSeparatorItem( itemInfo ) && !IsSubMenuItem( itemInfo ) && itemInfo.wID != 0; }

	inline std::tstring GetMenuItemText( const CMenu& menu, UINT itemId, UINT flags = MF_BYCOMMAND )
	{
		CString itemText;
		menu.GetMenuString( itemId, itemText, flags );
		return itemText.GetString();
	}

	unsigned int FindMenuItemIndex( const CMenu& rMenu, UINT itemId, unsigned int iFirst = 0 );
	unsigned int FindAfterMenuItemIndex( const CMenu& rMenu, UINT itemId, unsigned int iFirst = 0 );		// subsequent position

	void QueryMenuItemIds( std::vector< UINT >& rItemIds, const CMenu& rMenu );

	HMENU CloneMenu( HMENU hSrcMenu );

	size_t CopyMenuItems( CMenu& rDestMenu, unsigned int destIndex, const CMenu& srcMenu, const std::vector< UINT >* pSrcIds = NULL );
	void DeleteMenuItem( CMenu& rDestMenu, UINT itemId );
	void DeleteMenuItems( CMenu& rDestMenu, const UINT* pItemIds, size_t itemCount );

	void CleanupMenuDuplicates( CMenu& rDestMenu );
	void CleanupMenuSeparators( CMenu& rDestMenu );

	enum MenuInsert { PrependSrc, AppendSrc };

	bool JoinMenuItems( CMenu& rDestMenu, const CMenu& srcMenu, MenuInsert menuInsert = AppendSrc, bool addSep = true, UseMenuImages useMenuImages = NormalMenuImages );
}


namespace dbg
{
	void TraceMenu( HMENU hMenu, unsigned int indentLevel = 0 );
	void TraceMenuItem( HMENU hMenu, int itemPos );
	void TraceMenuItem( const MENUITEMINFO& itemInfo, int itemPos, unsigned int indentLevel = 0 );
}


#endif // MenuUtilities_h
