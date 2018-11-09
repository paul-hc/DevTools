#ifndef ShellContextMenu_h
#define ShellContextMenu_h
#pragma once

#include "ShellTypes.h"
#include "CmdIdStore.h"


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

	bool InvokeCommandByVerb( IContextMenu* pContextMenu, const char* pVerb, HWND hWnd );
	bool InvokeDefaultVerb( IContextMenu* pContextMenu, HWND hWnd );
	bool InvokeVerbOnItem( IShellItem* pShellItem, const wchar_t* pVerb, HWND hWnd );
}


namespace shell
{
	// Hosts and tracks a shell contextmenu of files, folders, shell-items. Calling clients can augment the popup menu with additional commands.
	//
	class CContextMenu : public CCmdTarget, private utl::noncopyable
	{
	public:
		CContextMenu( HWND hWndOwner, IContextMenu* pContextMenu = NULL );
		virtual ~CContextMenu();

		bool IsValid( void ) const { return m_pContextMenu != NULL; }

		void Reset( IContextMenu* pContextMenu = NULL );

		void SetFolderItem( IShellFolder* pParentFolder, PCITEMID_CHILD pidlItem );
		void SetFolderItems( IShellFolder* pParentFolder, PCUITEMID_CHILD_ARRAY pidlItemsArray, size_t itemCount );
		void SetItem( PCIDLIST_ABSOLUTE pidlAbs );
		void SetItem( IShellItem* pItem );
		void SetFilePath( const std::tstring& filePath );
		void SetFilePaths( const std::vector< std::tstring >& filePaths );

		// context popup menu
		enum { MinCmdId = 1, MaxCmdId = 0x7FFF, AtEnd = -1 };

		CMenu* GetPopupMenu( void ) { ASSERT_PTR( m_popupMenu.GetSafeHmenu() ); return &m_popupMenu; }
		void SetExternalPopupMenu( CMenu* pMenu );			// pass NULL for using internal popup menu

		bool HasShellCmds( void ) { return m_popupMenu.GetSafeHmenu() != NULL && !m_shellIdStore.IsEmpty(); }		// has it called QueryContextMenu() and there are common commands?
		bool HasShellCmd( int cmdId ) const { return m_shellIdStore.ContainsId( cmdId ); }

		bool MakePopupMenu( CMenu& rPopupMenu, int atIndex = AtEnd, UINT queryFlags = CMF_NORMAL | CMF_EXPLORE );
		bool MakePopupMenu( UINT queryFlags = CMF_NORMAL ) { return MakePopupMenu( m_popupMenu, AtEnd, queryFlags ); }

		int TrackMenu( const CPoint& screenPos, UINT atIndex = AtEnd, UINT queryFlags = CMF_NORMAL | CMF_EXPLORE );
		int TrackMenu( CMenu* pPopupMenu, const CPoint& screenPos );

		std::tstring GetItemVerb( int cmdId ) const;
		bool InvokeVerb( const char* pVerb );
		bool InvokeVerbIndex( int verbIndex ) { ASSERT( verbIndex >= 0 ); return InvokeVerb( MAKEINTRESOURCEA( verbIndex ) ); }
		bool InvokeDefaultVerb( void );

		static int ToVerbIndex( int cmdId ) { return cmdId - MinCmdId; }		// fix the misalignment of the verb with its cmdId
	protected:
		CMenu* EnsurePopupShellCmds( UINT queryFlags );
	private:
		LRESULT HandleWndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam );

		static LRESULT CALLBACK HookWndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam );

		enum MenuOwnership { InternalMenu, ExternalMenu };
	private:
		HWND m_hWndOwner;
		WNDPROC m_pOldWndProc;
		CMenu m_popupMenu;
		MenuOwnership m_menuOwnership;
		ui::CCmdIdStore m_shellIdStore;					// contains only commands belonging to the shell context menu
		CComPtr< IContextMenu > m_pContextMenu;

		// used for handling messages during menu tracking
		CComQIPtr< IContextMenu2 > m_pContextMenu2;
		CComQIPtr< IContextMenu3 > m_pContextMenu3;

		static CContextMenu* s_pInstance;				// one at a time

		// generated stuff
	public:
		virtual BOOL OnCmdMsg( UINT id, int code, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo );
	protected:
		DECLARE_MESSAGE_MAP()
	};
}


#endif // ShellContextMenu_h
