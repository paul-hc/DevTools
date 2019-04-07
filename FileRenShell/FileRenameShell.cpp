
#include "stdafx.h"
#include "FileRenShell.h"
#include "FileRenameShell.h"
#include "ShellMenuController.h"

#ifdef _DEBUG
	#include "utl/FlagTags.h"
	#define new DEBUG_NEW
#endif


CFileRenameShell::CFileRenameShell( void )
{
}

CFileRenameShell::~CFileRenameShell( void )
{
}

size_t CFileRenameShell::ExtractDropInfo( IDataObject* pSelFileObjects )
{
	ASSERT_PTR( pSelFileObjects );

	FORMATETC format = { CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
	STGMEDIUM stgMedium = { 0 };	// defend against buggy data object

	if ( HR_OK( pSelFileObjects->GetData( &format, &stgMedium ) ) )		// make the data transfer
	{
		m_pController.reset( new CShellMenuController( this, (HDROP)stgMedium.hGlobal ) );		// initialize the controller and file model
		return m_pController->GetFileModel().GetSourcePaths().size();
	}

	return 0;
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

STDMETHODIMP CFileRenameShell::Initialize( LPCITEMIDLIST folderPidl, IDataObject* pSelFileObjects, HKEY hKeyProgId )
{
	folderPidl, hKeyProgId;
	AFX_MANAGE_STATE( AfxGetStaticModuleState() )		// [PC 2018] required for loading resources

	//TRACE( _T("CFileRenameShell::Initialize()\n") );
	if ( pSelFileObjects != NULL )
	{
		//utl::CSectionGuard section( _T("CFileRenameShell::Initialize()") );

		ExtractDropInfo( pSelFileObjects );
	}
	return S_OK;
}


// IContextMenu interface implementation

STDMETHODIMP CFileRenameShell::QueryContextMenu( HMENU hMenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT flags )
{
	idCmdLast;

	AFX_MANAGE_STATE( AfxGetStaticModuleState() )		// [PC 2018] required for loading resources

	TRACE( _T(" CFileRenameShell::QueryContextMenu(): flags=0x%X {%s}\n"), flags, CShellMenuController::GetTags_ContextMenuFlags().FormatKey( flags ).c_str() );

	// res\FileRenameShell.rgs: this shell extension registers itself for both "*" and "lnkfile" types as ContextMenuHandlers.
	//		http://microsoft.public.platformsdk.shell.narkive.com/yr1YoK9e/obtaining-selected-shortcut-lnk-files-inside-ishellextinit-initialize
	//		https://stackoverflow.com/questions/21848694/windows-shell-extension-doesnt-give-exact-file-paths
	//
	if ( !HasFlag( flags, CMF_DEFAULTONLY ) &&			// kind of CMF_NORMAL
		 !HasFlag( flags, CMF_VERBSONLY ) )				// for "lnkfile": prevent menu item duplication due to querying twice (* and lnkfile)
	{
		if ( IsInit() )
		{
			//utl::CSectionGuard section( _T("CFileRenameShell::QueryContextMenu()") );

			if ( m_pController->EnsureCopyDropFilesAsPaths() )
				TRACE( _T(" CFileRenameShell::QueryContextMenu(): found files copied or cut on clipboard - also store their paths as text!\n") );

			UINT nextCmdId = m_pController->AugmentMenuItems( hMenu, indexMenu, idCmdFirst );

			TRACE( " CFileRenameShell::QueryContextMenu(): indexMenu=%d idCmdFirst=%d idCmdLast=%d  nextCmdId=%d  flags=0x%X\n", indexMenu, idCmdFirst, idCmdLast, nextCmdId, flags );
			return MAKE_HRESULT( SEVERITY_SUCCESS, FACILITY_NULL, nextCmdId );
		}
	}

	TRACE( _T(" ! CFileRenameShell::QueryContextMenu(): Ignoring add menu for flag: 0x%08X\n"), flags );
	return S_OK;
}

STDMETHODIMP CFileRenameShell::InvokeCommand( CMINVOKECOMMANDINFO* pCmi )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() )		// [PC 2018] required for loading resources

	// If HIWORD(pCmi->lpVerb) then we have been called programmatically and lpVerb is a command that should be invoked.
	// Otherwise, the shell has called us, and LOWORD(pCmi->lpVerb) is the menu ID the user has selected.
	// Actually, it's (menu ID - idCmdFirst) from QueryContextMenu().
	//
	if ( 0 == HIWORD( pCmi->lpVerb ) )
		if ( IsInit() )
		{
			//utl::CSectionGuard section( str::Format( _T("CFileRenameShell::InvokeCommand(): %d files"), m_pFileModel->GetSourcePaths().size() ) );

			if ( !m_pController->ExecuteCommand( LOWORD( pCmi->lpVerb ), pCmi->hwnd ) )
				ASSERT( false );			// unhandled command

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

	if ( flags == GCS_HELPTEXTA || flags == GCS_HELPTEXTW )
		if ( IsInit() )
		{
			std::tstring statusBarInfo;
			if ( m_pController->FindStatusBarInfo( statusBarInfo, idCmd ) )
			{
				_bstr_t statusInfo( statusBarInfo.c_str() );		// holds both ANSI and WIDE strings
				switch ( flags )
				{
					case GCS_HELPTEXTA: strncpy( pName, (const char*)statusInfo, cchMax ); break;
					case GCS_HELPTEXTW: wcsncpy( (wchar_t*)pName, (const wchar_t*)statusInfo, cchMax ); break;
				}
			}
		}

	return S_OK;
}
