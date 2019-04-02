#ifndef FileRenameShell_h
#define FileRenameShell_h
#pragma once

#include "Application_fwd.h"
#include "FileCommands_fwd.h"
#include "resource.h"


class CFileModel;
class CDropFilesModel;
class CBaseMenuBuilder;
interface IFileEditor;


class ATL_NO_VTABLE CFileRenameShell
	: public CComObjectRootEx< CComSingleThreadModel >
	, public CComCoClass< CFileRenameShell, &CLSID_FileRenameShell >
	, public ISupportErrorInfo
	, public IShellExtInit
	, public IContextMenu
{
public:
	CFileRenameShell( void );
	~CFileRenameShell();
private:
	size_t ExtractDropInfo( IDataObject* pDropInfo );

	UINT AugmentMenuItems( HMENU hMenu, UINT indexMenu, UINT idBaseCmd );		// return the added commands count
	HMENU BuildCreateFoldersSubmenu( CBaseMenuBuilder* pParentBuilder );
	HMENU BuildPasteDeepSubmenu( CBaseMenuBuilder* pParentBuilder );

	enum MenuCommand
	{
		Cmd_SendToCliboard,
		Cmd_RenameFiles, Cmd_TouchFiles, Cmd_FindDuplicates,
		Cmd_Undo, Cmd_Redo,
		Cmd_RunUnitTests,
		Cmd_CreateFolders,
		Cmd_CreateDeepFolderStruct,
		Cmd_PasteDeepBase,

		Cmd_Separator = -1,

		// popup IDs are negative
		Popup_PasteFolderStruct = -550,
		Popup_PasteDeep,
	};

	struct CMenuCmdInfo
	{
		MenuCommand m_cmd;
		const TCHAR* m_pTitle;
		const TCHAR* m_pStatusBarInfo;
		UINT m_iconId;
	};

	void ExecuteCommand( MenuCommand menuCmd, CWnd* pParentOwner );
	bool ExecutePasteDeep( MenuCommand menuCmd, CWnd* pParentOwner );

	CBitmap* MakeCmdInfo( std::tstring& rItemText, const CMenuCmdInfo& cmdInfo, const std::tstring& tabbedText = str::GetEmpty() );
	void AddCmd( CBaseMenuBuilder* pMenuBuilder, MenuCommand cmd, const std::tstring& tabbedText = str::GetEmpty() );
	static const CMenuCmdInfo* FindCmd( MenuCommand cmd );
	static const CMenuCmdInfo* FindCmd( MenuCommand cmd, const CMenuCmdInfo cmds[], size_t count );
private:
	std::auto_ptr< CFileModel > m_pFileModel;				// files selected in Explorer
	std::auto_ptr< CDropFilesModel > m_pDropFilesModel;		// files copied or cut to clipboard (CF_HDROP)

	static const CMenuCmdInfo s_commands[];
	static const CMenuCmdInfo s_moreCommands[];

	// generated COM stuff
public:
	DECLARE_REGISTRY_RESOURCEID( IDR_FILERENAMESHELL_REG )

	DECLARE_PROTECT_FINAL_CONSTRUCT()

	BEGIN_COM_MAP( CFileRenameShell )
		COM_INTERFACE_ENTRY( ISupportErrorInfo )
		COM_INTERFACE_ENTRY( IShellExtInit )
		COM_INTERFACE_ENTRY( IContextMenu )
	END_COM_MAP()
public:
	// ISupportErrorInfo
	STDMETHOD( InterfaceSupportsErrorInfo )( REFIID riid );

	// IShellExtInit
	STDMETHOD( Initialize )( LPCITEMIDLIST folderPidl, IDataObject* pDropInfo, HKEY hKeyProgId );

	// IContextMenu
	STDMETHOD( QueryContextMenu )( HMENU hMenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT flags );
	STDMETHOD( InvokeCommand )( CMINVOKECOMMANDINFO* pCmi );
	STDMETHOD( GetCommandString )( UINT_PTR idCmd, UINT flags, UINT* pReserved, LPSTR pName, UINT cchMax );
};


#endif // FileRenameShell_h
