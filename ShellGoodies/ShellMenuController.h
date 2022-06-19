#ifndef ShellMenuController_h
#define ShellMenuController_h
#pragma once

#include "Application_fwd.h"
#include "AppCommands.h"
#include "FileModel.h"


class CDropFilesModel;
class CBaseMenuBuilder;
class CSystemTray;
class CFlagTags;
interface IFileEditor;


class CShellMenuController		// instantiated while tracking the shell context menu
{
	enum MenuCommand;
public:
	CShellMenuController( IContextMenu* pContextMenu, HDROP hDropInfo );
	~CShellMenuController();

	CFileModel& GetFileModel( void ) { return m_fileModel; }

	bool EnsureCopyDropFilesAsPaths( void );
	UINT AugmentMenuItems( HMENU hMenu, UINT indexMenu, UINT idBaseCmd );		// return the added commands count
	bool ExecuteCommand( UINT cmdId, HWND hWnd );
	bool FindStatusBarInfo( std::tstring& rInfoText, UINT_PTR cmdId ) const;

	static const CFlagTags& GetTags_ContextMenuFlags( void );
private:
	HMENU BuildPasteDeepSubmenu( CBaseMenuBuilder* pParentBuilder );
	HMENU BuildCreateFoldersSubmenu( CBaseMenuBuilder* pParentBuilder );
	HMENU BuildMoreGoodiesSubmenu( CBaseMenuBuilder* pParentBuilder );

	enum MenuCommand
	{
		Cmd_SendToCliboard,
		Cmd_RenameFiles, Cmd_TouchFiles, Cmd_FindDuplicates,
		Cmd_Undo, Cmd_Redo,
		Cmd_Dashboard,
		Cmd_Options,
		Cmd_RunUnitTests,
		Cmd_CreateFolders,
		Cmd_CreateDeepFolderStruct,
		Cmd_PasteAsBackup,
		Cmd_PasteDeepBase,

		Cmd_Separator = -1,

		// popup IDs are negative
		Popup_PasteDeep = -550,
		Popup_PasteFolderStruct,
		Popup_MoreGoodies,
	};

	struct CMenuCmdInfo
	{
		MenuCommand m_cmd;
		const TCHAR* m_pTitle;
		const TCHAR* m_pStatusBarInfo;
		UINT m_iconId;
	};

	bool HandleCommand( MenuCommand menuCmd, CWnd* pParentOwner );
	bool HandlePasteDeepCmd( MenuCommand menuCmd, CWnd* pParentOwner );

	CBitmap* MakeCmdInfo( std::tstring& rItemText, const CMenuCmdInfo& cmdInfo, const std::tstring& tabbedText = str::GetEmpty() );
	bool AddCmd( CBaseMenuBuilder* pMenuBuilder, MenuCommand cmd, const std::tstring& tabbedText = str::GetEmpty() );
	static const CMenuCmdInfo* FindCmd( MenuCommand cmd );
	static const CMenuCmdInfo* FindCmd( MenuCommand cmd, const CMenuCmdInfo cmds[], size_t count );
private:
	IContextMenu* m_pContextMenu;
	CFileModel m_fileModel;								// files selected in Explorer
	std::auto_ptr<CDropFilesModel> m_pDropFilesModel;	// files cached (copied or cut) to clipboard (CF_HDROP)
	std::auto_ptr<CSystemTray> m_pSystemTray;			// system-tray shared popup window: hidden by default, used to display balloon notifications

	static const CMenuCmdInfo s_rootCommands[];			// commands to add directly to the context menu (not in a sub-menu)
	static const CMenuCmdInfo s_moreCommands[];			// commands to add in sub-menus, or depending on current context
};


#endif // ShellMenuController_h
