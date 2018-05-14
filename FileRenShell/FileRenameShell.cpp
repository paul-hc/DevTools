
#include "stdafx.h"
#include "FileRenShell.h"
#include "FileRenameShell.h"
#include "MainRenameDialog.h"
#include "utl/ImageStore.h"
#include "utl/Utilities.h"
#include "utl/resource.h"


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
	{ Cmd_RenameFiles, _T("&Rename Files..."), _T("Rename selected files in the dialog"), ID_RENAME_ITEM, false },
	{ Cmd_SendToCliboard, _T("&Send To Clipboard"), _T("Send the selected files path to clipboard"), ID_SEND_TO_CLIP, false },
	{ Cmd_TouchFiles, _T("&Touch Files..."), _T("Modify the timestamp of selected files"), ID_TOUCH_FILES, false },

	{ Cmd_RenameAndCopy, _T("C&opy..."), _T("Rename selected files and copy source paths to clipboard"), ID_RENAME_AND_COPY, false },
	{ Cmd_RenameAndCapitalize, _T("&Capitalize..."), _T("Rename selected files and capitalize filenames"), IDD_CAPITALIZE_OPTIONS, false },
	{ Cmd_RenameAndLowCaseExt, _T("&Low case extension..."), _T("Rename selected files and make extensions lower case"), IDC_CHANGE_CASE_BUTTON, false },
	{ Cmd_RenameAndReplace, _T("Re&place..."), _T("Rename selected files and replace text in filenames"), ID_EDIT_REPLACE, false },
	{ Cmd_RenameAndReplaceDelims, _T("Replace &Delimiters..."), _T("Rename selected files and replace all delimiters with spaces"), 0, false },

	{ Cmd_RenameAndSingleWhitespace, _T("&Single Whitespace...\t\"   \" to \" \""), _T("Rename selected files and replace multiple to single whitespace"), 0, true },
	{ Cmd_RenameAndRemoveWhitespace, _T("&Remove Whitespace...\t\"    \" to \"\""), _T("Rename selected files and remove whitespace"), 0, false },
	{ Cmd_RenameAndDashToSpace, _T("&Dash to Space...\t\"-\" to \" \""), _T("Rename selected files and replace dashes with spaces"), 0, false },
	{ Cmd_RenameAndUnderbarToSpace, _T("&Underbar to Space...\t\"_\" to \" \""), _T("Rename selected files and replace underbars to spaces"), 0, false },
	{ Cmd_RenameAndSpaceToUnderbar, _T("Space to Underbar...\t\" \" to \"_\""), _T("Rename selected files and replace spaces with underbars"), 0, false },

	{ Cmd_UndoRename, _T("&Undo Last Rename..."), _T("Undo last renamed files..."), IDD_CAPITALIZE_OPTIONS, true }
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

	return m_fileData.SetupFromDropInfo( (HDROP)storageMedium.hGlobal );
}

const CFileRenameShell::CMenuCmdInfo* CFileRenameShell::FindCmd( MenuCommand cmd )
{
	for ( int i = 0; i != COUNT_OF( m_commands ); ++i )
		if ( cmd == m_commands[ i ].m_cmd )
			return &m_commands[ i ];

	return NULL;
}

void CFileRenameShell::AugmentMenuItems( HMENU hMenu, UINT indexMenu, UINT idBaseCmd )
{
	HMENU hSubMenu = ::CreatePopupMenu();
	COLORREF menuColor = GetSysColor( COLOR_MENU );
	CImageStore* pImageStore = CImageStore::GetSharedStore();
	ASSERT_PTR( pImageStore );

	for ( int i = 0; i != COUNT_OF( m_commands ); ++i )
	{
		if ( m_commands[ i ].m_cmd < _CmdFirstSubMenu )
			::InsertMenu( hMenu, indexMenu++, MF_STRING | MF_BYPOSITION, idBaseCmd + m_commands[ i ].m_cmd, m_commands[ i ].m_pTitle );
		else
		{
			if ( m_commands[ i ].m_addSep )
				::AppendMenu( hSubMenu, MF_SEPARATOR, 0, NULL );

			::AppendMenu( hSubMenu, MF_STRING, idBaseCmd + m_commands[ i ].m_cmd, m_commands[ i ].m_pTitle );
		}

		if ( CBitmap* pMenuBitmap = pImageStore->RetrieveBitmap( m_commands[ i ].m_iconId, menuColor ) )
			SetMenuItemBitmaps( m_commands[ i ].m_cmd < _CmdFirstSubMenu ? hMenu : hSubMenu,
				idBaseCmd + m_commands[ i ].m_cmd,
				MF_BYCOMMAND,
				*pMenuBitmap,
				*pMenuBitmap );
	}

	// add sub-menu popup
	::InsertMenu( hMenu, indexMenu++, MF_POPUP | MF_STRING | MF_BYPOSITION, (UINT_PTR)hSubMenu, _T("Rename And") );

	if ( CBitmap* pMenuBitmap = pImageStore->RetrieveBitmap( ID_SHELL_SUBMENU, menuColor ) )
		SetMenuItemBitmaps( hMenu, (UINT)hSubMenu, MF_BYCOMMAND, *pMenuBitmap, *pMenuBitmap );
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
		//TRACE( _T("CFileRenameShell::QueryContextMenu(): selFileCount=%d\n"), m_fileData.GetFileCount() );

		if ( !m_fileData.IsEmpty() )
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
	if ( !m_fileData.IsEmpty() )
		if ( 0 == HIWORD( pCmi->lpVerb ) )
		{
			//TRACE( _T("CFileRenameShell::InvokeCommand(): selFileCount=%d\n"), m_fileData.GetFileCount() );

			MenuCommand menuCmd = static_cast< MenuCommand >( LOWORD( pCmi->lpVerb ) );
			CScopedMainWnd scopedMainWnd( pCmi->hwnd );

			if ( !scopedMainWnd.HasValidParentOwner() )
			{
				ui::BeepSignal( MB_ICONERROR );
				return S_OK;
			}

			switch ( menuCmd )
			{
				case Cmd_SendToCliboard:
					m_fileData.CopyClipSourcePaths( GetKeyState( VK_SHIFT ) & 0x8000 ? FilenameExt : FullPath, scopedMainWnd.m_pParentOwner );
					return S_OK;
				case Cmd_TouchFiles:
					//m_fileData.CopyClipSourcePaths( GetKeyState( VK_SHIFT ) & 0x8000 ? FilenameExt : FullPath, scopedMainWnd.m_pParentOwner );
					return S_OK;
				default:
				{
					m_fileData.LoadUndoLog();

					CMainRenameDialog dlg( menuCmd, &m_fileData, scopedMainWnd.m_pParentOwner );
					if ( IDOK == dlg.DoModal() )
						m_fileData.SaveUndoLog();

					return S_OK;
				}
			}
		}

	//TRACE( _T("CFileRenameShell::InvokeCommand(): Unrecognized command: %d\n") );
	return E_INVALIDARG;
}

STDMETHODIMP CFileRenameShell::GetCommandString( UINT_PTR idCmd, UINT flags, UINT* pReserved, LPSTR pName, UINT cchMax )
{
	pReserved, cchMax;
	AFX_MANAGE_STATE( AfxGetStaticModuleState() )

	//TRACE( _T("CFileRenameShell::GetCommandString(): flags=0x%08X, cchMax=%d\n"), flags, cchMax );

	if ( !m_fileData.IsEmpty() )
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
