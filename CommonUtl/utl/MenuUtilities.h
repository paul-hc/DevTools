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

	void LoadPopupMenu( CMenu& rContextMenu, UINT menuResId, int popupIndex, UseMenuImages useMenuImages = NormalMenuImages, std::tstring* pPopupText = NULL );

	bool SetMenuImages( CMenu& rMenu, bool useCheckedBitmaps = false, CImageStore* pImageStore = NULL );
	bool SetMenuItemImage( CMenu& rMenu, UINT itemId, UINT iconId = 0, bool useCheckedBitmaps = false, CImageStore* pImageStore = NULL );

	int TrackPopupMenu( CMenu& rMenu, CWnd* pTargetWnd, CPoint screenPos, UINT trackFlags = TPM_RIGHTBUTTON, const RECT* pExcludeRect = NULL );
	int TrackPopupMenuAlign( CMenu& rMenu, CWnd* pTargetWnd, const RECT& excludeRect, PopupAlign popupAlign = DropDown, UINT trackFlags = TPM_RIGHTBUTTON );


	CWnd* AutoTargetWnd( CWnd* pTargetWnd );
	bool AdjustMenuTrackPos( CPoint& rScreenPos );
	DWORD GetAlignTrackFlags( PopupAlign popupAlign );
	CPoint GetAlignTrackPos( PopupAlign popupAlign, const RECT& excludeRect );


	inline std::tstring GetMenuItemText( const CMenu& menu, UINT itemId, UINT flags = MF_BYCOMMAND )
	{
		CString itemText;
		menu.GetMenuString( itemId, itemText, flags );
		return itemText.GetString();
	}

	unsigned int FindMenuItemIndex( const CMenu& rMenu, UINT itemId, unsigned int iFirst = 0 );
	unsigned int FindAfterMenuItemIndex( const CMenu& rMenu, UINT itemId, unsigned int iFirst = 0 ); // subsequent position

	void QueryMenuItemIds( std::vector< UINT >& rItemIds, const CMenu& rMenu );

	HMENU CloneMenu( HMENU hSourceMenu );

	size_t CopyMenuItems( CMenu& rDestMenu, unsigned int destIndex, const CMenu& sourceMenu, const std::vector< UINT >* pSourceIds = NULL );
	void DeleteMenuItem( CMenu& rDestMenu, UINT itemId );
	void DeleteMenuItems( CMenu& rDestMenu, const UINT* pItemIds, size_t itemCount );

	void CleanupMenuDuplicates( CMenu& rDestMenu );
	void CleanupMenuSeparators( CMenu& rDestMenu );

	enum MenuInsert { PrependSrc, AppendSrc };

	bool JoinMenuItems( CMenu& rDestMenu, const CMenu& srcMenu, MenuInsert menuInsert = AppendSrc, bool addSep = true, UseMenuImages useMenuImages = NormalMenuImages );


	void SetRadio( CCmdUI* pCmdUI, BOOL checked = BST_CHECKED );
	bool ExpandVersionInfoTags( CCmdUI* pCmdUI );				// based on CVersionInfo
	void UpdateMenuUI( CWnd* pWindow, CMenu* pPopupMenu, bool autoMenuEnable = true );
};


#endif // MenuUtilities_h
