#ifndef ShellContextMenu_h
#define ShellContextMenu_h
#pragma once

#include "ShellTypes.h"


namespace shell
{
	// context menu of absolute item(s)
	CComPtr< IContextMenu > GetItemContextMenu( PCIDLIST_ABSOLUTE pidlAbs, HWND hWndOwner );
	CComPtr< IContextMenu > GetItemContextMenu( const std::tstring& filePath, HWND hWndOwner );
	CComPtr< IContextMenu > GetItemsContextMenu( const std::vector< std::tstring >& filePaths, HWND hWndOwner );

	// context menu of child item(s)
	CComPtr< IContextMenu > GetItemContextMenu( IShellFolder* pParentFolder, PCITEMID_CHILD pidlItem, HWND hWndOwner );
	CComPtr< IContextMenu > GetItemsContextMenu( IShellFolder* pParentFolder, PCUITEMID_CHILD_ARRAY pidlItemsArray, size_t itemCount, HWND hWndOwner );
	CComPtr< IContextMenu > GetItemContextMenu( IShellItem* pItem, HWND hWndOwner );

	HRESULT InvokeCommandByVerb( IContextMenu* pContextMenu, const char* pVerb, HWND hWnd );
	bool InvokeVerbOnItem( IShellItem* pShellItem, const wchar_t* pVerb, HWND hWnd );
}


namespace shell
{
	// Hosts and tracks a shell contextmenu of files, folders, shell-items. Calling clients can augment the popup menu with additional commands.
	//
	class CContextMenu : private utl::noncopyable
	{
	public:
		CContextMenu( HWND hWndOwner, IContextMenu* pContextMenu = NULL );
		virtual ~CContextMenu();

		bool IsValid( void ) const { return m_pContextMenu != NULL; }

		CMenu* GetPopupMenu( void ) { ASSERT_PTR( m_popupMenu.GetSafeHmenu() ); return &m_popupMenu; }
		void SetExternalPopupMenu( CMenu* pMenu );			// pass NULL for using internal popup menu

		void Reset( IContextMenu* pContextMenu = NULL );

		void SetFolderItem( IShellFolder* pParentFolder, PCITEMID_CHILD pidlItem );
		void SetFolderItems( IShellFolder* pParentFolder, PCUITEMID_CHILD_ARRAY pidlItemsArray, size_t itemCount );
		void SetItem( PCIDLIST_ABSOLUTE pidlAbs );
		void SetItem( IShellItem* pItem );
		void SetFilePath( const std::tstring& filePath );
		void SetFilePaths( const std::vector< std::tstring >& filePaths );

		enum { AtEnd = UINT_MAX, MinCmdId = 1, MaxCmdId = 0x7FFF };

		bool MakePopupMenu( CMenu& rPopupMenu, UINT atIndex = AtEnd, UINT flags = CMF_NORMAL | CMF_EXPLORE, UINT cmdIdFirst = MinCmdId, UINT cmdIdLast = MaxCmdId ) const;

		int TrackMenu( const CPoint& screenPos, UINT atIndex = AtEnd, UINT flags = CMF_NORMAL | CMF_EXPLORE );
		int TrackMenu( CMenu* pPopupMenu, const CPoint& screenPos );
	private:
		bool InvokeCommand( UINT cmdId );
		LRESULT HandleWndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam );

		static LRESULT CALLBACK HookWndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam );

		enum MenuOwnership { InternalMenu, ExternalMenu };
	private:
		HWND m_hWndOwner;
		WNDPROC m_pOldWndProc;
		CMenu m_popupMenu;
		MenuOwnership m_menuOwnership;
		CComPtr< IContextMenu > m_pContextMenu;

		// used for handling messages during menu tracking
		CComQIPtr< IContextMenu2 > m_pContextMenu2;
		CComQIPtr< IContextMenu3 > m_pContextMenu3;

		static CContextMenu* s_pInstance;				// one at a time
	};
}


#endif // ShellContextMenu_h
