#ifndef FileRenameShell_h
#define FileRenameShell_h
#pragma once

#include "Application_fwd.h"
#include "FileCommands_fwd.h"
#include "resource.h"


class CFileModel;
interface IFileEditor;

class ATL_NO_VTABLE CFileRenameShell
	: public CComObjectRootEx< CComSingleThreadModel >
	, public CComCoClass< CFileRenameShell, &CLSID_FileRenameShell >
	, public ISupportErrorInfo
	, public IDispatchImpl< IFileRenameShell, &IID_IFileRenameShell, &LIBID_FILERENSHELLLib >
	, public IShellExtInit
	, public IContextMenu
{
public:
	CFileRenameShell( void );
	~CFileRenameShell();
private:
	size_t ExtractDropInfo( IDataObject* pDropInfo );
	void AugmentMenuItems( HMENU hMenu, UINT indexMenu, UINT idBaseCmd );

	enum MenuCommand
	{
		Cmd_SendToCliboard, Cmd_RenameFiles, Cmd_TouchFiles, Cmd_Undo, Cmd_Redo,
		Cmd_RunUnitTests,
			_CmdCount
	};

	struct CMenuCmdInfo
	{
		MenuCommand m_cmd;
		const TCHAR* m_pTitle;
		const TCHAR* m_pStatusBarInfo;
		UINT m_iconId;
		bool m_addSep;
	};

	void ExecuteCommand( MenuCommand menuCmd, CWnd* pParentOwner );

	std::tstring FormatCmdText( const CMenuCmdInfo& cmdInfo );
	static const CMenuCmdInfo* FindCmd( MenuCommand cmd );
private:
	std::auto_ptr< CFileModel > m_pFileModel;
	static const CMenuCmdInfo m_commands[];
public:
	DECLARE_REGISTRY_RESOURCEID( IDR_FILERENAMESHELL_REG )

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
	STDMETHOD( Initialize )( LPCITEMIDLIST folderPidl, IDataObject* pDropInfo, HKEY hKeyProgId );

	// IContextMenu
	STDMETHOD( QueryContextMenu )( HMENU hMenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT flags );
	STDMETHOD( InvokeCommand )( CMINVOKECOMMANDINFO* pCmi );
	STDMETHOD( GetCommandString )( UINT_PTR idCmd, UINT flags, UINT* pReserved, LPSTR pName, UINT cchMax );
};


#endif // FileRenameShell_h
