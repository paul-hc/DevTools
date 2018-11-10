#ifndef ShellContextMenuHost_h
#define ShellContextMenuHost_h
#pragma once

#include "ShellTypes.h"
#include "CmdIdStore.h"


namespace shell
{
	// context menu of absolute item(s)
	CComPtr< IContextMenu > MakeAbsoluteContextMenu( PCIDLIST_ABSOLUTE pidlAbs, HWND hWndOwner );
	CComPtr< IContextMenu > MakeFilePathContextMenu( const std::tstring& filePath, HWND hWndOwner );
	CComPtr< IContextMenu > MakeFilePathsContextMenu( const std::vector< std::tstring >& filePaths, HWND hWndOwner );
	//CComPtr< IContextMenu > GetRelativeItemsContextMenu( const std::vector< fs::CPath >& filePaths, HWND hWndOwner );

	// context menu of child item(s)
	CComPtr< IContextMenu > MakeFolderItemContextMenu( IShellFolder* pParentFolder, PCITEMID_CHILD pidlItem, HWND hWndOwner );
	CComPtr< IContextMenu > MakeFolderItemsContextMenu( IShellFolder* pParentFolder, PCUITEMID_CHILD_ARRAY pidlItemsArray, size_t itemCount, HWND hWndOwner );
	CComPtr< IContextMenu > MakeItemContextMenu( IShellItem* pItem, HWND hWndOwner );

	bool InvokeCommandByVerb( IContextMenu* pContextMenu, const char* pVerb, CWnd* pWndOwner );
	bool InvokeDefaultVerb( IContextMenu* pContextMenu, CWnd* pWndOwner );
	bool InvokeVerbOnItem( IShellItem* pShellItem, const wchar_t* pVerb, HWND hWnd );
}


// Hosts and tracks a shell contextmenu of files, folders, shell-items. Calling clients can augment the popup menu with additional commands.
// Use shell::Make*ContextMenu() utility functions to make IContextMenu* for various datasets.
//
class CShellContextMenuHost : public CCmdTarget, private utl::noncopyable
{
public:
	CShellContextMenuHost( CWnd* pWndOwner, IContextMenu* pContextMenu = NULL );
	virtual ~CShellContextMenuHost();

	void Reset( IContextMenu* pContextMenu = NULL );

	bool IsValid( void ) const { return m_pContextMenu != NULL; }

	// context popup menu
	enum MenuOwnership { InternalMenu, ExternalMenu };

	CMenu* GetPopupMenu( void ) { ASSERT_PTR( m_popupMenu.GetSafeHmenu() ); return &m_popupMenu; }
	void SetPopupMenu( HMENU hMenu, MenuOwnership ownership = InternalMenu );			// pass NULL for using internal popup menu
	void DeletePopupMenu( void );

	enum { MinCmdId = 1, MaxCmdId = 0x7FFF, AtEnd = -1 };

	bool HasShellCmds( void ) { return !m_shellIdStore.IsEmpty(); }		// has it called QueryContextMenu(), are there common commands?
	bool HasShellCmd( int cmdId ) const { return m_shellIdStore.ContainsId( cmdId ); }

	bool MakePopupMenu( UINT queryFlags = CMF_NORMAL ) { return MakePopupMenu( m_popupMenu, AtEnd, queryFlags ); }
	bool MakePopupMenu( CMenu& rPopupMenu, int atIndex = AtEnd, UINT queryFlags = CMF_EXPLORE );						// external menu

	int TrackMenu( const CPoint& screenPos, UINT atIndex = AtEnd, UINT queryFlags = CMF_EXPLORE );
	int TrackMenu( CMenu* pPopupMenu, const CPoint& screenPos, UINT trackFlags = TPM_RETURNCMD | TPM_RIGHTBUTTON );		// external menu

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
private:
	CWnd* m_pWndOwner;
	WNDPROC m_pOldWndProc;
	CMenu m_popupMenu;
	MenuOwnership m_menuOwnership;
	ui::CCmdIdStore m_shellIdStore;					// contains only commands belonging to the shell context menu
	CComPtr< IContextMenu > m_pContextMenu;

	// used for handling messages during menu tracking
	CComQIPtr< IContextMenu2 > m_pContextMenu2;
	CComQIPtr< IContextMenu3 > m_pContextMenu3;

	static CShellContextMenuHost* s_pInstance;		// single one tracking at a time

	// generated stuff
public:
	virtual BOOL OnCmdMsg( UINT id, int code, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo );
protected:
	afx_msg void OnShellCommand( UINT cmdId );

	DECLARE_MESSAGE_MAP()
};


#endif // ShellContextMenuHost_h
