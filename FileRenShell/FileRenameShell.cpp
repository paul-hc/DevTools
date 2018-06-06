
#include "stdafx.h"
#include "FileRenShell.h"
#include "FileRenameShell.h"
#include "FileModel.h"
#include "MainRenameDialog.h"
#include "TouchFilesDialog.h"
#include "Application.h"
#include "utl/FmtUtils.h"
#include "utl/ImageStore.h"
#include "utl/Utilities.h"
#include "utl/resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


struct CScopedMainWnd
{
	CScopedMainWnd( HWND hWnd )
		: m_pParentOwner( NULL )
		, m_pOldMainWnd( NULL )
	{
		if ( hWnd != NULL && ::IsWindow( hWnd ) )
			m_pParentOwner = CWnd::FromHandle( hWnd )->GetTopLevelParent();

		if ( ::IsWindow( m_pParentOwner->GetSafeHwnd() ) )
			if ( CWnd* pOldMainWnd = AfxGetMainWnd() )
				if ( NULL == pOldMainWnd->m_hWnd )						// it happens sometimes, kind of transitory state when invoking from Explorer.exe
					if ( CWinThread* pCurrThread = AfxGetThread() )
					{
						m_pOldMainWnd = pOldMainWnd;
						pCurrThread->m_pMainWnd = m_pParentOwner;		// temporarily substitute main window
					}
	}

	~CScopedMainWnd()
	{
		if ( m_pOldMainWnd != NULL )
			if ( CWinThread* pCurrThread = AfxGetThread() )
				pCurrThread->m_pMainWnd = m_pOldMainWnd;				// restore original main window
	}

	bool HasValidParentOwner( void ) const { return IsWindow( m_pParentOwner->GetSafeHwnd() ) != FALSE; }
	bool IsValidMainWnd( void ) const { return HasValidParentOwner() && NULL == m_pOldMainWnd; }
public:
	CWnd* m_pParentOwner;
	CWnd* m_pOldMainWnd;
};


// CFileRenameShell implementation

const CFileRenameShell::CMenuCmdInfo CFileRenameShell::m_commands[] =
{
	{ Cmd_SendToCliboard, _T("&Send To Clipboard"), _T("Send the selected files path to clipboard"), ID_SEND_TO_CLIP, false },
	{ Cmd_RenameFiles, _T("&Rename Files..."), _T("Rename selected files in the dialog"), ID_RENAME_ITEM, false },
	{ Cmd_TouchFiles, _T("&Touch Files..."), _T("Modify the timestamp of selected files"), ID_TOUCH_FILES, false },
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
	HRESULT hResult = pDropInfo->GetData( &format, &storageMedium );		// make the data transfer
	if ( FAILED( hResult ) )
	{
		TRACE( _T("CFileRenameShell::ExtractDropInfo() failed: hResult=0x%08X\n"), hResult );
		return 0;
	}

	m_pFileModel.reset( new CFileModel );
	return m_pFileModel->SetupFromDropInfo( (HDROP)storageMedium.hGlobal );
}

void CFileRenameShell::AugmentMenuItems( HMENU hMenu, UINT indexMenu, UINT idBaseCmd )
{
	COLORREF menuColor = GetSysColor( COLOR_MENU );
	CImageStore* pImageStore = CImageStore::GetSharedStore();
	ASSERT_PTR( pImageStore );

	::InsertMenu( hMenu, indexMenu++, MF_SEPARATOR | MF_BYPOSITION, 0, NULL );

	for ( int i = 0; i != COUNT_OF( m_commands ); ++i )
	{
		std::tstring itemText = FormatCmdText( m_commands[ i ] );
		if ( !itemText.empty() )
		{
			if ( m_commands[ i ].m_addSep )
				::InsertMenu( hMenu, indexMenu++, MF_SEPARATOR | MF_BYPOSITION, 0, NULL );

			::InsertMenu( hMenu, indexMenu++, MF_STRING | MF_BYPOSITION, idBaseCmd + m_commands[ i ].m_cmd, itemText.c_str() );

			if ( CBitmap* pMenuBitmap = pImageStore->RetrieveBitmap( m_commands[ i ].m_iconId, menuColor ) )
				SetMenuItemBitmaps( hMenu,
					idBaseCmd + m_commands[ i ].m_cmd,
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
		m_pFileModel->LoadCommandModel();

		if ( utl::ICommand* pTopCmd = m_pFileModel->PeekCmdAs< utl::ICommand >( Cmd_Undo == cmdInfo.m_cmd ? cmd::Undo : cmd::Redo ) )
			return str::Format( cmdInfo.m_pTitle, pTopCmd->Format( true ).c_str() );

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
	m_pFileModel->LoadCommandModel();

	std::auto_ptr< IFileEditor > pFileEditor;
	switch ( menuCmd )
	{
		case Cmd_RenameFiles:
			pFileEditor.reset( m_pFileModel->MakeFileEditor( cmd::RenameFile, pParentOwner ) );
			break;
		case Cmd_TouchFiles:
			pFileEditor.reset( m_pFileModel->MakeFileEditor( cmd::TouchFile, pParentOwner ) );
			break;
		case Cmd_Undo:
		case Cmd_Redo:
			if ( CCommandModel* pCommandModel = m_pFileModel->GetCommandModel() )
				if ( utl::ICommand* pTopCmd = Cmd_Undo == menuCmd ? pCommandModel->PeekUndo() : pCommandModel->PeekRedo() )
					switch ( pTopCmd->GetTypeID() )
					{
						case cmd::RenameFile:
						case cmd::TouchFile:
							pFileEditor.reset( m_pFileModel->MakeFileEditor( static_cast< cmd::CommandType >( pTopCmd->GetTypeID() ), pParentOwner ) );
							pFileEditor->PopUndoRedoTop( Cmd_Undo == menuCmd ? cmd::Undo : cmd::Redo );
							break;
					}

			if ( NULL == pFileEditor.get() )
				ui::ReportError( _T("No command available to undo."), MB_OK | MB_ICONINFORMATION );
			break;
		default:
			ASSERT( false );
	}

	if ( pFileEditor.get() != NULL )
		if ( IDOK == pFileEditor->GetDialog()->DoModal() )
			m_pFileModel->SaveCommandModel();
}

const CFileRenameShell::CMenuCmdInfo* CFileRenameShell::FindCmd( MenuCommand cmd )
{
	for ( int i = 0; i != COUNT_OF( m_commands ); ++i )
		if ( cmd == m_commands[ i ].m_cmd )
			return &m_commands[ i ];

	return NULL;
}


// ISupportsErrorInfo interface implementation

STDMETHODIMP CFileRenameShell::InterfaceSupportsErrorInfo( REFIID riid )
{
	static const IID* interfaces[] = { &IID_IFileRenameShell, &__uuidof( IShellExtInit ), &IID_IContextMenu /*__uuidof( IContextMenu )*/ };

	for ( int i = 0; i != COUNT_OF( interfaces ); i++ )
		if ( InlineIsEqualGUID( *interfaces[ i ], riid ) )
			return S_OK;

	return S_FALSE;
}


// IFileRenameShell interface implementation


// IShellExtInit interface implementation

STDMETHODIMP CFileRenameShell::Initialize( LPCITEMIDLIST pidlFolder, IDataObject* pDropInfo, HKEY hKeyProgId )
{
	pidlFolder, hKeyProgId;
	AFX_MANAGE_STATE( AfxGetStaticModuleState() )

	TRACE( _T("CFileRenameShell::Initialize()\n") );
	if ( pDropInfo != NULL )
		ExtractDropInfo( pDropInfo );
	return S_OK;
}


// IContextMenu interface implementation

STDMETHODIMP CFileRenameShell::QueryContextMenu( HMENU hMenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT flags )
{
	idCmdLast;
	AFX_MANAGE_STATE( AfxGetStaticModuleState() )

	if ( !( flags & CMF_DEFAULTONLY ) )
	{	// kind of CMF_NORMAL
		//TRACE( _T("CFileRenameShell::QueryContextMenu(): selFileCount=%d\n"), m_fileData.GetSourceFiles().size() );

		if ( m_pFileModel.get() != NULL )
		{
			AugmentMenuItems( hMenu, indexMenu, idCmdFirst );
			return MAKE_HRESULT( SEVERITY_SUCCESS, FACILITY_NULL, _CmdCount );
		}
	}

	//TRACE( _T("CFileRenameShell::QueryContextMenu(): Ignoring add menu for flag: 0x%08X\n"), flags );
	return S_OK;
}

STDMETHODIMP CFileRenameShell::InvokeCommand( LPCMINVOKECOMMANDINFO pCmi )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() )

	// If HIWORD( pCmi->lpVerb ) then we have been called programmatically
	// and lpVerb is a command that should be invoked.	Otherwise, the shell
	// has called us, and LOWORD( pCmi->lpVerb ) is the menu ID the user has
	// selected.  Actually, it's (menu ID - idCmdFirst) from QueryContextMenu().
	if ( m_pFileModel.get() != NULL )
		if ( 0 == HIWORD( pCmi->lpVerb ) )
		{
			//TRACE( _T("CFileRenameShell::InvokeCommand(): selFileCount=%d\n"), m_pFileModel->GetSourcePaths().size() );

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
	AFX_MANAGE_STATE( AfxGetStaticModuleState() )

	//TRACE( _T("CFileRenameShell::GetCommandString(): flags=0x%08X, cchMax=%d\n"), flags, cchMax );

	if ( m_pFileModel.get() != NULL )
		if ( flags == GCS_HELPTEXTA || flags == GCS_HELPTEXTW )
			if ( const CMenuCmdInfo* pCmdInfo = FindCmd( static_cast< MenuCommand >( idCmd ) ) )
			{
				_bstr_t statusInfo( pCmdInfo->m_pStatusBarInfo );
				switch ( flags )
				{
					case GCS_HELPTEXTA: strcpy( pName, (const char*)statusInfo ); break;
					case GCS_HELPTEXTW: wcscpy( (wchar_t*)pName, (const wchar_t*)statusInfo ); break;
				}
			}

	return S_OK;
}
