#ifndef ShellContextMenuHost_h
#define ShellContextMenuHost_h
#pragma once

#include "Path_fwd.h"
#include "ui_fwd.h"
#include "CmdIdStore.h"
#include <shobjidl_core.h>		// for IContextMenu


namespace shell
{
	// context menu of absolute item(s)
	CComPtr<IContextMenu> MakeFilePathContextMenu( const TCHAR* pShellPath, HWND hWndOwner );
	CComPtr<IContextMenu> MakeAbsoluteContextMenu( PCIDLIST_ABSOLUTE pidlAbs, HWND hWndOwner );

	// context menu of child item(s)
	CComPtr<IContextMenu> MakeFolderItemContextMenu( IShellFolder* pParentFolder, PCUITEMID_CHILD pidlItem, HWND hWndOwner );
	CComPtr<IContextMenu> MakeFolderItemsContextMenu( IShellFolder* pParentFolder, PCUITEMID_CHILD_ARRAY pidlItemsArray, size_t itemCount, HWND hWndOwner );
	CComPtr<IContextMenu> MakeItemContextMenu( IShellItem* pItem, HWND hWndOwner );

	bool InvokeCommandByVerb( IContextMenu* pContextMenu, const char* pVerb, CWnd* pWndOwner );
	bool InvokeDefaultVerb( IContextMenu* pContextMenu, CWnd* pWndOwner );
	bool InvokeVerbOnItem( IShellItem* pShellItem, const wchar_t* pVerb, HWND hWnd );
}


// Hosts and tracks a shell contextmenu of files, folders, shell-items. Calling clients can augment the popup menu with additional commands.
// Use shell::Make*ContextMenu() utility functions to make IContextMenu* for various datasets.
//
class CShellContextMenuHost : public CCmdTarget
							, private utl::noncopyable
{
public:
	CShellContextMenuHost( CWnd* pWndOwner, IContextMenu* pContextMenu = nullptr );
	virtual ~CShellContextMenuHost();

	bool IsValid( void ) const { return m_pContextMenu != nullptr || IsLazyUninit(); }
	IContextMenu* Get( void ) const { return m_pContextMenu; }

	void Reset( IContextMenu* pContextMenu = nullptr );

	// context popup menu
	enum MenuOwnership { InternalMenu, ExternalMenu };

	CMenu* GetPopupMenu( void ) { ASSERT_PTR( m_contextMenu.GetSafeHmenu() ); return &m_contextMenu; }
	void SetPopupMenu( HMENU hMenu, MenuOwnership ownership = InternalMenu );			// pass NULL for using internal popup menu
	void DeletePopupMenu( void );

	enum { AtEnd = -1 };

	bool HasShellCmds( void ) { return !m_shellIdStore.IsEmpty(); }		// has it called QueryContextMenu(), are there common commands?
	bool HasShellCmd( int cmdId ) const { return m_shellIdStore.ContainsId( cmdId ); }

	bool MakePopupMenu( UINT queryFlags = CMF_NORMAL ) { return MakePopupMenu( m_contextMenu, AtEnd, queryFlags ); }		// internal menu
	bool MakePopupMenu( CMenu& rPopupMenu, int atIndex = AtEnd, UINT queryFlags = CMF_EXPLORE );

	int TrackMenu( const CPoint& screenPos, UINT atIndex = AtEnd, UINT queryFlags = CMF_EXPLORE );						// internal menu
	virtual int TrackMenu( CMenu* pPopupMenu, const CPoint& screenPos, UINT trackFlags = TPM_RETURNCMD | TPM_RIGHTBUTTON );

	std::tstring GetItemVerb( int cmdId ) const;
	bool InvokeVerb( const char* pVerb );
	bool InvokeVerbIndex( int verbIndex );
	bool InvokeDefaultVerb( void );

	static int ToVerbIndex( int cmdId ) { return cmdId - ui::MinCmdId; }		// fix the misalignment of the verb with its cmdId
protected:
	virtual bool IsLazyUninit( void ) const;			// for delayed IContextMenu query - kind of don't know if invalid
	int DoTrackMenu( CMenu* pPopupMenu, const CPoint& screenPos, UINT trackFlags );
	CMenu* EnsurePopupShellCmds( UINT queryFlags );
protected:
	class CTrackingHook
	{
	public:
		CTrackingHook( HWND hWndOwner, IContextMenu* pContextMenu );
		~CTrackingHook();
	private:
		LRESULT HandleWndProc2( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam );
		LRESULT HandleWndProc3( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam );

		static LRESULT CALLBACK HookWndProc2( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam );
		static LRESULT CALLBACK HookWndProc3( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam );
	private:
		HWND m_hWndOwner;
		WNDPROC m_pOldWndProc;
		CComQIPtr<IContextMenu2> m_pContextMenu2;
		CComQIPtr<IContextMenu3> m_pContextMenu3;

		static CTrackingHook* s_pInstance;			// single one tracking at a time
	};
protected:
	CWnd* m_pWndOwner;
private:
	CMenu m_contextMenu;
	MenuOwnership m_menuOwnership;
	ui::CCmdIdStore m_shellIdStore;					// contains only commands belonging to the shell context menu
	CComPtr<IContextMenu> m_pContextMenu;

	// generated stuff
public:
	virtual BOOL OnCmdMsg( UINT id, int code, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo );
protected:
	afx_msg void OnShellCommand( UINT cmdId );

	DECLARE_MESSAGE_MAP()
};


// Allows lazy initialization of the "Explorer" sub-menu, only when the user requires it (for faster context menu tracking).
//
class CShellLazyContextMenuHost : public CShellContextMenuHost
{
	friend class CExplorerSubMenuHook;
public:
	CShellLazyContextMenuHost( CWnd* pWndOwner, IContextMenu* pContextMenu = nullptr, UINT queryFlags = CMF_NORMAL );
	virtual ~CShellLazyContextMenuHost();

	void StoreShellPaths( const std::vector<shell::TPath>& shellPaths ) { REQUIRE( !m_isLazyInit ); m_shellPaths = shellPaths; }

	// base overrides
	virtual int TrackMenu( CMenu* pPopupMenu, const CPoint& screenPos, UINT trackFlags = TPM_RETURNCMD | TPM_RIGHTBUTTON );
protected:
	virtual bool IsLazyUninit( void ) const;
	bool LazyInit( void );
private:
	UINT m_queryFlags;
	bool m_isLazyInit;
	OPTIONAL std::vector<shell::TPath> m_shellPaths;				// paths to query the context menu on lazy init, unless the object is build with an existing context menu
	std::auto_ptr<CExplorerSubMenuHook> m_pExplorerSubMenuHook;		// for lazy init: monitors when "Explorer" sub-menu gets expanded first time
	std::auto_ptr<CTrackingHook> m_pTrackingHook;
};


#endif // ShellContextMenuHost_h
