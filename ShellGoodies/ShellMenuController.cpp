
#include "stdafx.h"
#include "ShellMenuController.h"
#include "IFileEditor.h"
#include "DropFilesModel.h"
#include "Application.h"
#include "CmdDashboardDialog.h"
#include "OptionsSheet.h"
#include "utl/EnumTags.h"
#include "utl/FlagTags.h"
#include "utl/FmtUtils.h"
#include "utl/FileSystem_fwd.h"
#include "utl/Guards.h"
#include "utl/StringUtilities.h"
#include "utl/UI/Clipboard.h"
#include "utl/UI/ImageStore.h"
#include "utl/UI/ShellContextMenuBuilder.h"
#include "utl/UI/Utilities.h"
#include "utl/UI/resource.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


const CShellMenuController::CMenuCmdInfo CShellMenuController::s_commands[] =
{
	{ Cmd_Separator },
	{ Cmd_SendToCliboard, _T("&Send To Clipboard"), _T("Send the selected files path to clipboard"), ID_SEND_TO_CLIP },
	{ Cmd_RenameFiles, _T("&Rename Files..."), _T("Rename selected files in the dialog"), ID_RENAME_ITEM },
	{ Cmd_TouchFiles, _T("&Touch Files..."), _T("Modify the timestamp of selected files"), ID_TOUCH_FILES },
	{ Cmd_FindDuplicates, _T("Find &Duplicates..."), _T("Find duplicate files in selected folders or files, and allow the deletion of duplicates"), ID_FIND_DUPLICATE_FILES },
	{ Cmd_Undo, _T("&Undo \"%s\" ..."), _T("Undo last operation on files"), ID_EDIT_UNDO },
	{ Cmd_Redo, _T("&Redo \"%s\" ..."), _T("Redo last undo operation on files"), ID_EDIT_REDO },
	{ Popup_PasteFolderStruct, _T("Paste &Folder Structure"), _T("Open menu for creating folder structure based on files copied on clipboard"), ID_PASTE_FOLDER_STRUCT_POPUP },
	{ Popup_PasteDeep, _T("Paste D&eep"), _T("Paste copied files creating a deep folder"), ID_PASTE_DEEP_POPUP },
	{ Popup_MoreGoodies, _T("More &Goodies"), _T("Open more options"), 0 },
#ifdef _DEBUG
	{ Cmd_Separator },
	{ Cmd_RunUnitTests, _T("# Run Unit Tests (ShellGoodies)"), _T("Run the unit tests (debug build only)"), ID_RUN_TESTS },
#endif
	{ Cmd_Separator }
};

const CShellMenuController::CMenuCmdInfo CShellMenuController::s_moreCommands[] =
{
	{ Cmd_CreateFolders, _T("Create &Folders"), _T("Create folders in destination to replicate copied folders"), ID_CREATE_FOLDERS },
	{ Cmd_CreateDeepFolderStruct, _T("Create Deep Folder Structure"), _T("Create folders in destination to replicate copied folders and their sub-folders"), ID_CREATE_DEEP_FOLDER_STRUCT },
	{ Cmd_Dashboard, _T("Dashboard.."), _T("Open the undo/redo actions dashboard"), ID_OPEN_CMD_DASHBOARD },
	{ Cmd_Options, _T("&Properties..."), _T("Open the properties dialog"), ID_OPTIONS }
};


CShellMenuController::CShellMenuController( IContextMenu* pContextMenu, HDROP hDropInfo )
	: m_pContextMenu( pContextMenu )
	, m_fileModel( app::GetCmdSvc() )
{
	ASSERT_PTR( m_pContextMenu );
	ASSERT_PTR( hDropInfo );

	m_fileModel.SetupFromDropInfo( hDropInfo );

	if ( m_fileModel.IsSourceSingleFolder() )					// single selected directory as paste target?
		if ( CClipboard::HasDropFiles() )
		{
			m_pDropFilesModel.reset( new CDropFilesModel( m_fileModel.GetSourcePaths().front() ) );
			m_pDropFilesModel->BuildFromClipboard();
		}
}

CShellMenuController::~CShellMenuController()
{
}

bool CShellMenuController::EnsureCopyDropFilesAsPaths( void )
{
	CWnd* pParent = AfxGetMainWnd();
	return pParent != NULL && CClipboard::AlsoCopyDropFilesAsPaths( pParent );		// true if files are Copied or Cut on clipboard, and their paths is cached as text
}

UINT CShellMenuController::AugmentMenuItems( HMENU hMenu, UINT indexMenu, UINT idBaseCmd )
{
	CShellContextMenuBuilder menuBuilder( hMenu, indexMenu, idBaseCmd );

	for ( int i = 0; i != COUNT_OF( s_commands ); ++i )
	{
		const CMenuCmdInfo& cmdInfo = s_commands[ i ];

		if ( Cmd_Separator == cmdInfo.m_cmd )
			menuBuilder.AddSeparator();
		else
		{
			std::tstring itemText;
			CBitmap* pItemBitmap = MakeCmdInfo( itemText, cmdInfo );

			if ( !itemText.empty() )
				switch ( cmdInfo.m_cmd )
				{
					case Popup_PasteFolderStruct:
						if ( HMENU hSubMenu = BuildCreateFoldersSubmenu( &menuBuilder ) )
							menuBuilder.AddPopupItem( hSubMenu, itemText, pItemBitmap );
						break;
					case Popup_PasteDeep:
						if ( HMENU hSubMenu = BuildPasteDeepSubmenu( &menuBuilder ) )
						{
							itemText += str::Format( _T(" (%s)"), m_pDropFilesModel->FormatDropCounts().c_str() );
							menuBuilder.AddPopupItem( hSubMenu, itemText, pItemBitmap );
						}
						break;
					case Popup_MoreGoodies:
						if ( HMENU hSubMenu = BuildMoreGoodiesSubmenu( &menuBuilder ) )
							menuBuilder.AddPopupItem( hSubMenu, itemText, pItemBitmap );
						break;
					default:
						menuBuilder.AddCmdItem( cmdInfo.m_cmd, itemText, pItemBitmap );
				}
		}
	}

	return menuBuilder.GetNextCmdId();
}

HMENU CShellMenuController::BuildCreateFoldersSubmenu( CBaseMenuBuilder* pParentBuilder )
{
	if ( m_pDropFilesModel.get() != NULL )
		if ( m_pDropFilesModel->HasSrcFolderPaths() )
		{
			CSubMenuBuilder subMenuBuilder( pParentBuilder );

			AddCmd( &subMenuBuilder, Cmd_CreateFolders, str::Format( _T("(%d)"), m_pDropFilesModel->GetSrcFolderPaths().size() ) );

			if ( m_pDropFilesModel->HasSrcDeepFolderPaths() )
				AddCmd( &subMenuBuilder, Cmd_CreateDeepFolderStruct, str::Format( _T("(%d)"), m_pDropFilesModel->GetSrcDeepFolderPaths().size() ) );

			return subMenuBuilder.GetPopupMenu()->Detach();
		}

	return NULL;
}

HMENU CShellMenuController::BuildPasteDeepSubmenu( CBaseMenuBuilder* pParentBuilder )
{
	if ( m_pDropFilesModel.get() != NULL )
		if ( m_pDropFilesModel->HasDropPaths() && m_pDropFilesModel->HasRelFolderPathSeq() )
		{
			CSubMenuBuilder subMenuBuilder( pParentBuilder );

			for ( UINT i = 0, count = (UINT)m_pDropFilesModel->GetRelFolderPathSeq().size(); i != count; ++i )
			{
				std::tstring itemText;
				CBitmap* pFolderBitmap = m_pDropFilesModel->GetRelFolderItemInfo( itemText, i );

				subMenuBuilder.AddCmdItem( Cmd_PasteDeepBase + i, itemText, pFolderBitmap );
			}

			return subMenuBuilder.GetPopupMenu()->Detach();
		}

	return NULL;
}

HMENU CShellMenuController::BuildMoreGoodiesSubmenu( CBaseMenuBuilder* pParentBuilder )
{
	CSubMenuBuilder subMenuBuilder( pParentBuilder );

	AddCmd( &subMenuBuilder, Cmd_Dashboard );
	subMenuBuilder.AddSeparator();
	AddCmd( &subMenuBuilder, Cmd_Options );

	return subMenuBuilder.GetPopupMenu()->Detach();
}

CBitmap* CShellMenuController::MakeCmdInfo( std::tstring& rItemText, const CMenuCmdInfo& cmdInfo, const std::tstring& tabbedText /*= str::GetEmpty()*/ )
{
	switch ( cmdInfo.m_cmd )
	{
		case Cmd_SendToCliboard:
			rItemText = cmdInfo.m_pTitle;
	#ifdef _DEBUG
			rItemText += _T("  [DEBUG]");
	#endif
			break;
		case Cmd_Undo:
		case Cmd_Redo:
			if ( utl::ICommand* pTopCmd = app::GetCmdSvc()->PeekCmd( Cmd_Undo == cmdInfo.m_cmd ? svc::Undo : svc::Redo ) )
				rItemText = str::Format( cmdInfo.m_pTitle, pTopCmd->Format( utl::Detailed ).c_str() );
			else
				rItemText.clear();
			break;
		default:
			rItemText = cmdInfo.m_pTitle;
	}

	if ( !tabbedText.empty() )
		rItemText += _T('\t') + tabbedText;

	if ( cmdInfo.m_iconId != 0 )
		return CImageStore::SharedStore()->RetrieveMenuBitmap( cmdInfo.m_iconId );

	return NULL;
}

bool CShellMenuController::FindStatusBarInfo( std::tstring& rInfoText, UINT_PTR cmdId ) const
{
	MenuCommand menuCmd = static_cast<MenuCommand>( cmdId );

	if ( const CMenuCmdInfo* pCmdInfo = FindCmd( menuCmd ) )
	{
		rInfoText = pCmdInfo->m_pStatusBarInfo;
		switch ( menuCmd )
		{
			case Cmd_Undo:
			case Cmd_Redo:
				if ( utl::ICommand* pTopCmd = app::GetCmdSvc()->PeekCmd( Cmd_Undo == menuCmd ? svc::Undo : svc::Redo ) )
					stream::Tag( rInfoText, pTopCmd->Format( utl::Detailed ), _T(" ") );
		}
		return true;
	}

	rInfoText.clear();
	return false;
}

void CShellMenuController::AddCmd( CBaseMenuBuilder* pMenuBuilder, MenuCommand cmd, const std::tstring& tabbedText /*= str::GetEmpty()*/ )
{
	ASSERT_PTR( pMenuBuilder );

	const CMenuCmdInfo* pCmdInfo = FindCmd( cmd );
	ASSERT_PTR( pCmdInfo );

	std::tstring itemText;
	CBitmap* pItemBitmap = MakeCmdInfo( itemText, *pCmdInfo, tabbedText );

	pMenuBuilder->AddCmdItem( cmd, itemText, pItemBitmap );
}

bool CShellMenuController::ExecuteCommand( UINT cmdId, HWND hWnd )
{
	MenuCommand menuCmd = static_cast<MenuCommand>( cmdId );
	CScopedMainWnd scopedMainWnd( hWnd );

	if ( !scopedMainWnd.HasValidParentOwner() )
		return ui::BeepSignal( MB_ICONERROR );

	return HandleCommand( menuCmd, scopedMainWnd.GetParentOwnerWnd() );
}

bool CShellMenuController::HandleCommand( MenuCommand menuCmd, CWnd* pParentOwner )
{
	switch ( menuCmd )
	{
		case Cmd_SendToCliboard:
			m_fileModel.CopyClipSourcePaths( ui::IsKeyPressed( VK_SHIFT ) ? fmt::FilenameExt : fmt::FullPath, pParentOwner );
			return true;
		case Cmd_RunUnitTests:
			app::GetApp().RunUnitTests();
			return true;
	}

	// file operations commands
	std::auto_ptr<IFileEditor> pFileEditor;
	switch ( menuCmd )
	{
		case Cmd_RenameFiles:
			pFileEditor.reset( m_fileModel.MakeFileEditor( cmd::RenameFile, pParentOwner ) );
			break;
		case Cmd_TouchFiles:
			pFileEditor.reset( m_fileModel.MakeFileEditor( cmd::TouchFile, pParentOwner ) );
			break;
		case Cmd_FindDuplicates:
			pFileEditor.reset( m_fileModel.MakeFileEditor( cmd::FindDuplicates, pParentOwner ) );
			break;
		case Cmd_Undo:
		case Cmd_Redo:
		{
			svc::StackType stackType = Cmd_Undo == menuCmd ? svc::Undo : svc::Redo;
			std::pair<IFileEditor*, bool> editorPair = m_fileModel.HandleUndoRedo( stackType, pParentOwner );
			if ( editorPair.second )				// command handled?
				return true;

			if ( editorPair.first != NULL )			// we've got an editor to undo/redo?
			{
				pFileEditor.reset( editorPair.first );
				pFileEditor->PopStackTop( stackType );
			}
			else
			{
				ui::ReportError( str::Format( _T("No command available to %s."), svc::GetTags_StackType().FormatUi( stackType ).c_str() ).c_str(), MB_OK | MB_ICONINFORMATION );
				return true;						// reported
			}

			break;
		}
		case Cmd_Dashboard:
		{
			CCmdDashboardDialog dlg( &m_fileModel, svc::Undo, pParentOwner );
			dlg.DoModal();
			return true;
		}
		case Cmd_Options:
		{
			COptionsSheet sheet( &m_fileModel, pParentOwner );
			sheet.DoModal();
			return true;
		}
		case Cmd_CreateFolders:
		case Cmd_CreateDeepFolderStruct:
			if ( m_pDropFilesModel.get() != NULL )
			{
				m_pDropFilesModel->CreateFolders( Cmd_CreateFolders == menuCmd ? Shallow : Deep );
				return true;
			}
			break;
		default:
			return HandlePasteDeepCmd( menuCmd, pParentOwner );
	}

	if ( pFileEditor.get() != NULL )
	{
		// Note: CPathItemListCtrl::ResetShellContextMenu() that releases the hosted context menu (and this instance), leaving the UI with dangling pointers into m_pFileModel.
		CComPtr<IContextMenu> pThisAlive( m_pContextMenu );		// keep this shell extension COM object alive to make the UI code re-entrant

		pFileEditor->GetDialog()->DoModal();
		pFileEditor.reset();							// delete the dialog here, before pThis goes out of scope
		return true;
	}

	return false;
}

bool CShellMenuController::HandlePasteDeepCmd( MenuCommand menuCmd, CWnd* pParentOwner )
{
	if ( menuCmd >= Cmd_PasteDeepBase && m_pDropFilesModel.get() != NULL )
	{
		UINT relFldPos = menuCmd - Cmd_PasteDeepBase;
		if ( relFldPos < m_pDropFilesModel->GetRelFolderPathSeq().size() )
		{
			if ( !m_pDropFilesModel->PasteDeep( m_pDropFilesModel->GetRelFolderPathSeq()[ relFldPos ], pParentOwner ) )
				ui::BeepSignal();		// a file transfer error occured

			return true;				// handled
		}
	}
	return false;
}

const CShellMenuController::CMenuCmdInfo* CShellMenuController::FindCmd( MenuCommand cmd )
{
	if ( const CMenuCmdInfo* pFoundCmd = FindCmd( cmd, ARRAY_PAIR( s_commands ) ) )
		return pFoundCmd;

	return FindCmd( cmd, ARRAY_PAIR( s_moreCommands ) );
}

const CShellMenuController::CMenuCmdInfo* CShellMenuController::FindCmd( MenuCommand cmd, const CMenuCmdInfo cmds[], size_t count )
{
	for ( size_t i = 0; i != count; ++i )
		if ( cmd == cmds[ i ].m_cmd )
			return &cmds[ i ];

	return NULL;
}


#ifdef _DEBUG

const CFlagTags& CShellMenuController::GetTags_ContextMenuFlags( void )
{
	static const CFlagTags::FlagDef flagDefs[] =
	{
		{ FLAG_TAG( CMF_DEFAULTONLY ) },
		{ FLAG_TAG( CMF_VERBSONLY ) },
		{ FLAG_TAG( CMF_EXPLORE ) },
		{ FLAG_TAG( CMF_NOVERBS ) },
		{ FLAG_TAG( CMF_CANRENAME ) },
		{ FLAG_TAG( CMF_NODEFAULT ) },
		{ FLAG_TAG( CMF_ITEMMENU ) },
		{ FLAG_TAG( CMF_EXTENDEDVERBS ) },
		{ FLAG_TAG( CMF_DISABLEDVERBS ) },
		{ FLAG_TAG( CMF_ASYNCVERBSTATE ) },
		{ FLAG_TAG( CMF_OPTIMIZEFORINVOKE ) },
		{ FLAG_TAG( CMF_SYNCCASCADEMENU ) },
		{ FLAG_TAG( CMF_DONOTPICKDEFAULT ) }
	};
	static const CFlagTags s_tags( ARRAY_PAIR( flagDefs ) );
	return s_tags;
}

#endif //_DEBUG
