
#include "stdafx.h"
#include "FileRenShell.h"
#include "FileRenameShell.h"
#include "FileModel.h"
#include "IFileEditor.h"
#include "Application.h"
#include "DropFilesModel.h"
#include "utl/FmtUtils.h"
#include "utl/FileSystem_fwd.h"
#include "utl/Guards.h"
#include "utl/StringUtilities.h"
#include "utl/UI/Clipboard.h"
#include "utl/UI/ImageStore.h"
#include "utl/UI/ShellContextMenuBuilder.h"
#include "utl/UI/Utilities.h"
#include "utl/UI/resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


const CFileRenameShell::CMenuCmdInfo CFileRenameShell::s_commands[] =
{
	{ Cmd_Separator },
	{ Cmd_SendToCliboard, _T("&Send To Clipboard"), _T("Send the selected files path to clipboard"), ID_SEND_TO_CLIP },
	{ Cmd_RenameFiles, _T("&Rename Files..."), _T("Rename selected files in the dialog"), ID_RENAME_ITEM },
	{ Cmd_TouchFiles, _T("&Touch Files..."), _T("Modify the timestamp of selected files"), ID_TOUCH_FILES },
	{ Cmd_FindDuplicates, _T("Find &Duplicates..."), _T("Find duplicate files in selected folders or files, and allow the deletion of duplicates"), ID_FIND_DUPLICATE_FILES },
	{ Cmd_Undo, _T("&Undo %s..."), _T("Undo last operation on files"), ID_EDIT_UNDO },
	{ Cmd_Redo, _T("&Redo %s..."), _T("Redo last undo operation on files"), ID_EDIT_REDO },
	{ Popup_CreateFolders, _T("Create &Folder Structure"), _T("Popup menu for creating folder structure based on files copied on clipboard"), ID_CREATE_FOLDERS_POPUP },
	{ Popup_PasteDeep, _T("&Paste Deep"), _T("Paste copied files creating a deep folder"), ID_PASTE_DEEP_POPUP },
#ifdef _DEBUG
	{ Cmd_Separator },
	{ Cmd_RunUnitTests, _T("# Run Unit Tests (FileRenameShell)"), _T("Run the unit tests (debug build only)"), ID_RUN_TESTS },
#endif
	{ Cmd_Separator }
};

const CFileRenameShell::CMenuCmdInfo CFileRenameShell::s_moreCommands[] =
{
	{ Cmd_MakeFolders, _T("Create &Folders"), _T("Create folders in destination to replicate copied folders"), ID_MAKE_FOLDERS },
	{ Cmd_MakeDeepFolderStructure, _T("Create Deep Folder Structure"), _T("Create folders in destination to replicate copied folders and their sub-folders"), ID_MAKE_DEEP_FOLDER_STRUCTURE }
};


CFileRenameShell::CFileRenameShell( void )
{
}

CFileRenameShell::~CFileRenameShell( void )
{
}

size_t CFileRenameShell::ExtractDropInfo( IDataObject* pDropInfo )
{
	ASSERT_PTR( pDropInfo );

	FORMATETC format = { CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
	STGMEDIUM storageMedium;
	if ( HR_OK( pDropInfo->GetData( &format, &storageMedium ) ) )		// make the data transfer
	{
		m_pFileModel.reset( new CFileModel( app::GetCmdSvc() ) );
		m_pFileModel->SetupFromDropInfo( (HDROP)storageMedium.hGlobal );

		if ( 1 == m_pFileModel->GetSourcePaths().size() && fs::IsValidDirectory( m_pFileModel->GetSourcePaths().front().GetPtr() ) )	// single selected directory as paste target?
			if ( CClipboard::HasDropFiles() )
			{
				m_pDropFilesModel.reset( new CDropFilesModel( m_pFileModel->GetSourcePaths().front() ) );
				m_pDropFilesModel->BuildFromClipboard();
			}

		return m_pFileModel->GetSourcePaths().size();
	}

	return 0;
}

UINT CFileRenameShell::AugmentMenuItems( HMENU hMenu, UINT indexMenu, UINT idBaseCmd )
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
					case Popup_CreateFolders:
						if ( HMENU hSubMenu = BuildCreateFoldersSubmenu( &menuBuilder ) )
							menuBuilder.AddPopupItem( hSubMenu, itemText, pItemBitmap );
						break;
					case Popup_PasteDeep:
						if ( HMENU hSubMenu = BuildPasteDeepSubmenu( &menuBuilder, itemText ) )
							menuBuilder.AddPopupItem( hSubMenu, itemText, pItemBitmap );
						break;
					default:
						menuBuilder.AddCmdItem( cmdInfo.m_cmd, itemText, pItemBitmap );
				}
		}
	}

	return menuBuilder.GetAddedCmdCount();
}

HMENU CFileRenameShell::BuildCreateFoldersSubmenu( CBaseMenuBuilder* pParentBuilder )
{
	if ( m_pDropFilesModel.get() != NULL )
		if ( m_pDropFilesModel->HasSrcFolderPaths() )
		{
			CSubMenuBuilder subMenuBuilder( pParentBuilder );

			AddCmd( &subMenuBuilder, Cmd_MakeFolders, str::Format( _T("(%d)"), m_pDropFilesModel->GetSrcFolderPaths().size() ) );

			if ( m_pDropFilesModel->HasSrcDeepFolderPaths() )
				AddCmd( &subMenuBuilder, Cmd_MakeDeepFolderStructure, str::Format( _T("(%d)"), m_pDropFilesModel->GetSrcDeepFolderPaths().size() ) );

			return subMenuBuilder.GetPopupMenu()->Detach();
		}

	return NULL;
}

HMENU CFileRenameShell::BuildPasteDeepSubmenu( CBaseMenuBuilder* pParentBuilder, std::tstring& rItemText )
{
	if ( m_pDropFilesModel.get() != NULL )
		if ( m_pDropFilesModel->HasDropPaths() && m_pDropFilesModel->HasRelFolderPathSeq() )
		{
			rItemText += str::Format( _T(" (%s)"), m_pDropFilesModel->FormatDropCounts().c_str() );

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

CBitmap* CFileRenameShell::MakeCmdInfo( std::tstring& rItemText, const CMenuCmdInfo& cmdInfo, const std::tstring& tabbedText /*= str::GetEmpty()*/ )
{
	switch ( cmdInfo.m_cmd )
	{
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

void CFileRenameShell::AddCmd( CBaseMenuBuilder* pMenuBuilder, MenuCommand cmd, const std::tstring& tabbedText /*= str::GetEmpty()*/ )
{
	ASSERT_PTR( pMenuBuilder );

	const CMenuCmdInfo* pCmdInfo = FindCmd( cmd );
	ASSERT_PTR( pCmdInfo );

	std::tstring itemText;
	CBitmap* pItemBitmap = MakeCmdInfo( itemText, *pCmdInfo, tabbedText );

	pMenuBuilder->AddCmdItem( cmd, itemText, pItemBitmap );
}

void CFileRenameShell::ExecuteCommand( MenuCommand menuCmd, CWnd* pParentOwner )
{
	switch ( menuCmd )
	{
		case Cmd_SendToCliboard:
			m_pFileModel->CopyClipSourcePaths( GetKeyState( VK_SHIFT ) & 0x8000 ? fmt::FilenameExt : fmt::FullPath, pParentOwner );
			return;
		case Cmd_RunUnitTests:
			app::GetApp().RunUnitTests();
			return;
	}

	// file operations commands
	std::auto_ptr< IFileEditor > pFileEditor;
	switch ( menuCmd )
	{
		case Cmd_RenameFiles:
			pFileEditor.reset( m_pFileModel->MakeFileEditor( cmd::RenameFile, pParentOwner ) );
			break;
		case Cmd_TouchFiles:
			pFileEditor.reset( m_pFileModel->MakeFileEditor( cmd::TouchFile, pParentOwner ) );
			break;
		case Cmd_FindDuplicates:
			pFileEditor.reset( m_pFileModel->MakeFileEditor( cmd::FindDuplicates, pParentOwner ) );
			break;
		case Cmd_Undo:
		case Cmd_Redo:
		{
			svc::StackType stackType = Cmd_Undo == menuCmd ? svc::Undo : svc::Redo;
			svc::ICommandService* pCmdSvc = app::GetCmdSvc();
			if ( utl::ICommand* pTopCmd = pCmdSvc->PeekCmd( stackType ) )
				switch ( pTopCmd->GetTypeID() )
				{
					case cmd::RenameFile:
					case cmd::TouchFile:
						pFileEditor.reset( m_pFileModel->MakeFileEditor( static_cast< cmd::CommandType >( pTopCmd->GetTypeID() ), pParentOwner ) );
						pFileEditor->PopStackTop( Cmd_Undo == menuCmd ? svc::Undo : svc::Redo );
						break;
				}

			if ( NULL == pFileEditor.get() )
				ui::ReportError( _T("No command available to undo."), MB_OK | MB_ICONINFORMATION );
			break;
		}
		case Cmd_MakeFolders:
		case Cmd_MakeDeepFolderStructure:
			if ( m_pDropFilesModel.get() != NULL )
				m_pDropFilesModel->CreateFolders( Cmd_MakeFolders == menuCmd ? Shallow : Deep );
			else
				ASSERT( false );
			break;
		default:
			if ( !ExecutePasteDeep( menuCmd, pParentOwner ) )
				ASSERT( false );
	}

	if ( pFileEditor.get() != NULL )
	{
		// Note that CPathItemListCtrl::ResetShellContextMenu() that releases the hosted context menu (and this instance), leaving the UI with dangling pointers into m_pFileModel.
		CComPtr< IContextMenu > pThisAlive( this );		// keep this alive to make the UI code re-entrant

		pFileEditor->GetDialog()->DoModal();
		pFileEditor.reset();							// delete the dialog here, before pThis goes out of scope
	}
}

bool CFileRenameShell::ExecutePasteDeep( MenuCommand menuCmd, CWnd* pParentOwner )
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

const CFileRenameShell::CMenuCmdInfo* CFileRenameShell::FindCmd( MenuCommand cmd )
{
	if ( const CMenuCmdInfo* pFoundCmd = FindCmd( cmd, ARRAY_PAIR( s_commands ) ) )
		return pFoundCmd;

	return FindCmd( cmd, ARRAY_PAIR( s_moreCommands ) );
}

const CFileRenameShell::CMenuCmdInfo* CFileRenameShell::FindCmd( MenuCommand cmd, const CMenuCmdInfo cmds[], size_t count )
{
	for ( size_t i = 0; i != count; ++i )
		if ( cmd == cmds[ i ].m_cmd )
			return &cmds[ i ];

	return NULL;
}


// ISupportErrorInfo interface implementation

STDMETHODIMP CFileRenameShell::InterfaceSupportsErrorInfo( REFIID riid )
{
	static const IID* s_pInterfaceIds[] = { &__uuidof( IShellExtInit ), &__uuidof( IContextMenu )/*, &__uuidof( IFileRenameShell )*/ };		// aka &IID_IContextMenu

	for ( unsigned int i = 0; i != COUNT_OF( s_pInterfaceIds ); ++i )
		if ( ::InlineIsEqualGUID( *s_pInterfaceIds[ i ], riid ) )
			return S_OK;

	return S_FALSE;
}


// IShellExtInit interface implementation

STDMETHODIMP CFileRenameShell::Initialize( LPCITEMIDLIST folderPidl, IDataObject* pDropInfo, HKEY hKeyProgId )
{
	folderPidl, hKeyProgId;
	AFX_MANAGE_STATE( AfxGetStaticModuleState() )		// [PC 2018] required for loading resources

	//TRACE( _T("CFileRenameShell::Initialize()\n") );
	if ( pDropInfo != NULL )
	{
		//utl::CSectionGuard section( _T("CFileRenameShell::Initialize()") );

		ExtractDropInfo( pDropInfo );
	}
	return S_OK;
}


// IContextMenu interface implementation

STDMETHODIMP CFileRenameShell::QueryContextMenu( HMENU hMenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT flags )
{
	idCmdLast;

	AFX_MANAGE_STATE( AfxGetStaticModuleState() )		// [PC 2018] required for loading resources

	// res\FileRenameShell.rgs: this shell extension registers itself for both "*" and "lnkfile" types as ContextMenuHandlers.
	//		http://microsoft.public.platformsdk.shell.narkive.com/yr1YoK9e/obtaining-selected-shortcut-lnk-files-inside-ishellextinit-initialize
	//		https://stackoverflow.com/questions/21848694/windows-shell-extension-doesnt-give-exact-file-paths

	if ( !HasFlag( flags, CMF_DEFAULTONLY ) &&			// kind of CMF_NORMAL
		 !HasFlag( flags, CMF_VERBSONLY ) )				// for "lnkfile": prevent menu item duplication due to querying twice (* and lnkfile)
	{
		if ( m_pFileModel.get() != NULL )
		{
			//utl::CSectionGuard section( _T("CFileRenameShell::QueryContextMenu()") );

			if ( CWnd* pParent = AfxGetMainWnd() )
				if ( CClipboard::AlsoCopyDropFilesAsPaths( pParent ) )		// if files Copied or Cut on clipboard, also store their paths as text
					TRACE( _T("CFileRenameShell::QueryContextMenu(): found files copied or cut on clipboard - also store their paths as text!\n") );

			UINT cmdCount = AugmentMenuItems( hMenu, indexMenu, idCmdFirst );
			return MAKE_HRESULT( SEVERITY_SUCCESS, FACILITY_NULL, cmdCount );
		}
	}

	//TRACE( _T("CFileRenameShell::QueryContextMenu(): Ignoring add menu for flag: 0x%08X\n"), flags );
	return S_OK;
}

STDMETHODIMP CFileRenameShell::InvokeCommand( CMINVOKECOMMANDINFO* pCmi )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() )		// [PC 2018] required for loading resources

	// If HIWORD(pCmi->lpVerb) then we have been called programmatically and lpVerb is a command that should be invoked.
	// Otherwise, the shell has called us, and LOWORD(pCmi->lpVerb) is the menu ID the user has selected.
	// Actually, it's (menu ID - idCmdFirst) from QueryContextMenu().

	if ( m_pFileModel.get() != NULL )
		if ( 0 == HIWORD( pCmi->lpVerb ) )
		{
			//utl::CSectionGuard section( str::Format( _T("CFileRenameShell::InvokeCommand(): %d files"), m_pFileModel->GetSourcePaths().size() ) );

			MenuCommand menuCmd = static_cast< MenuCommand >( LOWORD( pCmi->lpVerb ) );
			CScopedMainWnd scopedMainWnd( pCmi->hwnd );

			if ( !scopedMainWnd.HasValidParentOwner() )
				ui::BeepSignal( MB_ICONERROR );
			else
				ExecuteCommand( menuCmd, scopedMainWnd.m_pParentOwner );

			return S_OK;
		}

	//TRACE( _T("CFileRenameShell::InvokeCommand(): Unrecognized command: %d\n") );
	return E_INVALIDARG;
}

STDMETHODIMP CFileRenameShell::GetCommandString( UINT_PTR idCmd, UINT flags, UINT* pReserved, LPSTR pName, UINT cchMax )
{
	pReserved, cchMax;
	AFX_MANAGE_STATE( AfxGetStaticModuleState() )		// [PC 2018] required for loading resources

	//TRACE( _T("CFileRenameShell::GetCommandString(): flags=0x%08X, cchMax=%d\n"), flags, cchMax );

	if ( m_pFileModel.get() != NULL )
		if ( flags == GCS_HELPTEXTA || flags == GCS_HELPTEXTW )
			if ( const CMenuCmdInfo* pCmdInfo = FindCmd( static_cast< MenuCommand >( idCmd ) ) )
			{
				//utl::CSectionGuard section( str::Format( _T("CFileRenameShell::GetCommandString(): id=%d, flags=0x%08X"), idCmd, flags ) );

				_bstr_t statusInfo( pCmdInfo->m_pStatusBarInfo );
				switch ( flags )
				{
					case GCS_HELPTEXTA: strcpy( pName, (const char*)statusInfo ); break;
					case GCS_HELPTEXTW: wcscpy( (wchar_t*)pName, (const wchar_t*)statusInfo ); break;
				}
			}

	return S_OK;
}
