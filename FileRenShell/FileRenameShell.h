#ifndef FileRenameShell_h
#define FileRenameShell_h
#pragma once

#include "Application_fwd.h"
#include "FileWorkingSet.h"
#include "resource.h"


class ATL_NO_VTABLE CFileRenameShell :
	public CComObjectRootEx< CComSingleThreadModel >,
	public CComCoClass< CFileRenameShell, &CLSID_FileRenameShell >,
	public ISupportErrorInfo,
	public IDispatchImpl< IFileRenameShell, &IID_IFileRenameShell, &LIBID_FILERENSHELLLib >,
	public IShellExtInit,
	public IContextMenu
{
public:
	CFileRenameShell( void );
	~CFileRenameShell();
private:
	size_t ExtractDropInfo( IDataObject* pDropInfo );
private:
	struct CMenuCmdInfo
	{
		app::MenuCommand m_cmd;
		const TCHAR* m_pTitle;
		const TCHAR* m_pStatusBarInfo;
		UINT m_iconId;
		bool m_addSep;
	};

	static const CMenuCmdInfo* FindCmd( app::MenuCommand cmd );
	void ExecuteCommand( app::MenuCommand menuCmd, CWnd* pParentOwner );
	void AugmentMenuItems( HMENU hMenu, UINT indexMenu, UINT idBaseCmd );
private:
	CFileWorkingSet m_fileData;
	static const CMenuCmdInfo m_commands[];
public:
	DECLARE_REGISTRY_RESOURCEID( IDR_FILERENAMESHELL )

	DECLARE_PROTECT_FINAL_CONSTRUCT()

	BEGIN_COM_MAP( CFileRenameShell )
		COM_INTERFACE_ENTRY( IFileRenameShell )
		COM_INTERFACE_ENTRY( IDispatch )
		COM_INTERFACE_ENTRY( ISupportErrorInfo )
		COM_INTERFACE_ENTRY( IShellExtInit )
		COM_INTERFACE_ENTRY( IContextMenu )
	END_COM_MAP()
public:
	// ISupportsErrorInfo
	STDMETHOD( InterfaceSupportsErrorInfo )( REFIID riid );

	// IFileRenameShell

	// IShellExtInit
	STDMETHOD( Initialize )( LPCITEMIDLIST pidlFolder, IDataObject* pDropInfo, HKEY hKeyProgId );

	// IContextMenu
	STDMETHOD( QueryContextMenu )( HMENU hMenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT flags );
	STDMETHOD( InvokeCommand )( LPCMINVOKECOMMANDINFO pCmi );
	STDMETHOD( GetCommandString )( UINT_PTR idCmd, UINT flags, UINT* pReserved, LPSTR pName, UINT cchMax );
};


#endif // FileRenameShell_h
