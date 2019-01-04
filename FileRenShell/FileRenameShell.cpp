
#include "stdafx.h"
#include "FileRenShell.h"
#include "FileRenameShell.h"
#include "FileModel.h"
#include "IFileEditor.h"
#include "Application.h"
#include "utl/FmtUtils.h"
#include "utl/Guards.h"
#include "utl/UI/ImageStore.h"
#include "utl/UI/Utilities.h"
#include "utl/UI/resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


const CFileRenameShell::CMenuCmdInfo CFileRenameShell::s_commands[] =
{
	{ Cmd_SendToCliboard, _T("&Send To Clipboard"), _T("Send the selected files path to clipboard"), ID_SEND_TO_CLIP, false },
	{ Cmd_RenameFiles, _T("&Rename Files..."), _T("Rename selected files in the dialog"), ID_RENAME_ITEM, false },
	{ Cmd_TouchFiles, _T("&Touch Files..."), _T("Modify the timestamp of selected files"), ID_TOUCH_FILES, false },
	{ Cmd_FindDuplicates, _T("Find &Duplicates..."), _T("Find duplicate files in selected folders or files, and allow the deletion of duplicates"), ID_FIND_DUPLICATE_FILES, false },
	{ Cmd_Undo, _T("&Undo %s..."), _T("Undo last operation on files"), ID_EDIT_UNDO, false },
	{ Cmd_Redo, _T("&Redo %s..."), _T("Redo last undo operation on files"), ID_EDIT_REDO, false },
#ifdef _DEBUG
	{ Cmd_RunUnitTests, _T("# Run Unit Tests (FileRenameShell)"), _T("Modify the timestamp of selected files"), ID_RUN_TESTS, true }
#endif
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
	if ( !HR_OK( pDropInfo->GetData( &format, &storageMedium ) ) )		// make the data transfer
		return 0;

	m_pFileModel.reset( new CFileModel( app::GetCmdSvc() ) );
	return m_pFileModel->SetupFromDropInfo( (HDROP)storageMedium.hGlobal );
}

void CFileRenameShell::AugmentMenuItems( HMENU hMenu, UINT indexMenu, UINT idBaseCmd )
{
	COLORREF menuColor = GetSysColor( COLOR_MENU );
	CImageStore* pImageStore = CImageStore::GetSharedStore();
	ASSERT_PTR( pImageStore );

	::InsertMenu( hMenu, indexMenu++, MF_SEPARATOR | MF_BYPOSITION, 0, NULL );

	static const MenuCommand firstCmdUsingDlg = Cmd_RenameFiles;

	for ( int i = 0; i != COUNT_OF( s_commands ); ++i )
	{
		std::tstring itemText = FormatCmdText( s_commands[ i ] );
		if ( !itemText.empty() )
		{
			if ( s_commands[ i ].m_addSep )
				::InsertMenu( hMenu, indexMenu++, MF_SEPARATOR | MF_BYPOSITION, 0, NULL );

			::InsertMenu( hMenu, indexMenu++, MF_STRING | MF_BYPOSITION, idBaseCmd + s_commands[ i ].m_cmd, itemText.c_str() );

			if ( CBitmap* pMenuBitmap = pImageStore->RetrieveBitmap( s_commands[ i ].m_iconId, menuColor ) )
				SetMenuItemBitmaps( hMenu,
					idBaseCmd + s_commands[ i ].m_cmd,
					MF_BYCOMMAND,
					*pMenuBitmap, *pMenuBitmap );
		}
	}

	::InsertMenu( hMenu, indexMenu++, MF_SEPARATOR | MF_BYPOSITION, 0, NULL );
}

std::tstring CFileRenameShell::FormatCmdText( const CMenuCmdInfo& cmdInfo )
{
	if ( Cmd_Undo == cmdInfo.m_cmd || Cmd_Redo == cmdInfo.m_cmd )
	{
		if ( utl::ICommand* pTopCmd = app::GetCmdSvc()->PeekCmd( Cmd_Undo == cmdInfo.m_cmd ? svc::Undo : svc::Redo ) )
			return str::Format( cmdInfo.m_pTitle, pTopCmd->Format( utl::Detailed ).c_str() );

		return std::tstring();
	}

	return cmdInfo.m_pTitle;
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
		default:
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

const CFileRenameShell::CMenuCmdInfo* CFileRenameShell::FindCmd( MenuCommand cmd )
{
	for ( int i = 0; i != COUNT_OF( s_commands ); ++i )
		if ( cmd == s_commands[ i ].m_cmd )
			return &s_commands[ i ];

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

			AugmentMenuItems( hMenu, indexMenu, idCmdFirst );
			return MAKE_HRESULT( SEVERITY_SUCCESS, FACILITY_NULL, _CmdCount );
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
