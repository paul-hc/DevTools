
#include "stdafx.h"
#include "ShellContextMenu.h"
#include "ContainerUtilities.h"
#include "MenuUtilities.h"
#include "Utilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace shell
{
	CComPtr< IContextMenu > GetItemContextMenu( PCIDLIST_ABSOLUTE pidlAbs, HWND hWndOwner )
	{
		CComPtr< IShellFolder > pParentFolder;
		PCUITEMID_CHILD pidlItem;			// no ownership (no allocation)
		::SHBindToParent( pidlAbs, IID_IShellFolder, (void**)&pParentFolder, &pidlItem );		// absolute pidl: get its parent folder and its relative PIDL

		return GetItemContextMenu( pParentFolder, pidlItem, hWndOwner );
	}

	CComPtr< IContextMenu > GetItemContextMenu( const std::tstring& filePath, HWND hWndOwner )
	{
		CComHeapPtr< ITEMIDLIST_ABSOLUTE > pidlItem( static_cast< ITEMIDLIST_ABSOLUTE* >( ::ILCreateFromPath( filePath.c_str() ) ) );	// 64 bit: prevent warning C4090: 'argument' : different '__unaligned' qualifiers
		return GetItemContextMenu( pidlItem, hWndOwner );
	}

	CComPtr< IContextMenu > GetItemsContextMenu( const std::vector< std::tstring >& filePaths, HWND hWndOwner )
	{
		CComPtr< IContextMenu > pCtxMenu;

		std::vector< PITEMID_CHILD > pidlItemsArray;
		if ( CComPtr< IShellFolder > pParentFolder = MakePidlArray( pidlItemsArray, filePaths ) )
			pCtxMenu = GetItemsContextMenu( &*pParentFolder, (PCUITEMID_CHILD_ARRAY)&pidlItemsArray.front(), pidlItemsArray.size(), hWndOwner );

		ClearOwningPidls( pidlItemsArray );
		return pCtxMenu;
	}


	CComPtr< IContextMenu > GetItemContextMenu( IShellFolder* pParentFolder, PCITEMID_CHILD pidlItem, HWND hWndOwner )
	{
		ASSERT_PTR( pParentFolder );

		CComPtr< IContextMenu > pCtxMenu;
		HR_AUDIT( pParentFolder->GetUIObjectOf( hWndOwner, 1, &pidlItem, __uuidof( IContextMenu ), NULL, (void**)&pCtxMenu ) );
		return pCtxMenu;
	}

	CComPtr< IContextMenu > GetItemContextMenu( IShellItem* pItem, HWND hWndOwner )
	{
		ASSERT_PTR( pItem );

		CComHeapPtr< ITEMID_CHILD > childPidl;
		if ( CComPtr< IShellFolder2 > pParentFolder = GetParentFolderAndPidl( &childPidl, pItem ) )
			return GetItemContextMenu( pParentFolder, childPidl, hWndOwner );

		return NULL;
	}

	CComPtr< IContextMenu > GetItemsContextMenu( IShellFolder* pParentFolder, PCUITEMID_CHILD_ARRAY pidlItemsArray, size_t itemCount, HWND hWndOwner )
	{
		ASSERT_PTR( pParentFolder );

		CComPtr< IContextMenu > pCtxMenu;
		HR_AUDIT( pParentFolder->GetUIObjectOf( hWndOwner, static_cast< unsigned int >( itemCount ), pidlItemsArray, __uuidof( IContextMenu ), NULL, (void**)&pCtxMenu ) );
		return pCtxMenu;
	}


	bool InvokeCommandByVerb( IContextMenu* pContextMenu, const char* pVerb, HWND hWnd )
	{	// hosts an IContextMenu and invokes a verb
		ASSERT_PTR( pContextMenu );

		CContextMenu ctxMenu( hWnd, pContextMenu );
		return
			ctxMenu.MakePopupMenu( CMF_VERBSONLY ) &&		// just verbs
			ctxMenu.InvokeVerb( pVerb );
	}

	bool InvokeDefaultVerb( IContextMenu* pContextMenu, HWND hWnd )
	{
		ASSERT_PTR( pContextMenu );

		CContextMenu ctxMenu( hWnd, pContextMenu );
		return ctxMenu.InvokeDefaultVerb();
	}


	bool InvokeVerbOnItem( IShellItem* pShellItem, const wchar_t* pVerb, HWND hWnd )
	{
		ASSERT_PTR( pShellItem );

		CComHeapPtr< ITEMIDLIST_ABSOLUTE > pidl;
		if ( !HR_OK( ::SHGetIDListFromObject( pShellItem, &pidl ) ) )
			return false;

		SHELLEXECUTEINFO info;
		utl::ZeroWinStruct( &info );

		info.fMask = SEE_MASK_INVOKEIDLIST | SEE_MASK_IDLIST | SEE_MASK_UNICODE;
		info.lpIDList = pidl;
		info.lpVerb = pVerb;
		info.hwnd = hWnd;
		info.nShow = SW_SHOWNORMAL;

		return ::ShellExecuteEx( &info ) != FALSE;
	}
}


namespace shell
{
	CContextMenu* CContextMenu::s_pInstance = NULL;

	CContextMenu::CContextMenu( HWND hWndOwner, IContextMenu* pContextMenu /*= NULL*/ )
		: m_hWndOwner( hWndOwner )
		, m_pOldWndProc( NULL )
		, m_menuOwnership( InternalMenu )
	{
		m_popupMenu.CreatePopupMenu();
		Reset( pContextMenu );

		ASSERT( NULL == s_pInstance );
		s_pInstance = this;
	}

	CContextMenu::~CContextMenu()
	{
		Reset();

		if ( ExternalMenu == m_menuOwnership )
			m_popupMenu.Detach();			// avoid destroying externally owned menu

		ASSERT( this == s_pInstance );
		s_pInstance = NULL;
	}

	void CContextMenu::Reset( IContextMenu* pContextMenu /*= NULL*/ )
	{
		m_pContextMenu = pContextMenu;

		// for handling menu messages while tracking
		m_pContextMenu2 = m_pContextMenu;
		m_pContextMenu3 = m_pContextMenu;
	}

	void CContextMenu::SetFolderItem( IShellFolder* pParentFolder, PCITEMID_CHILD pidlItem )
	{
		Reset( shell::GetItemContextMenu( pParentFolder, pidlItem, m_hWndOwner ) );
	}

	void CContextMenu::SetFolderItems( IShellFolder* pParentFolder, PCUITEMID_CHILD_ARRAY pidlItemsArray, size_t itemCount )
	{
		Reset( shell::GetItemsContextMenu( pParentFolder, pidlItemsArray, itemCount, m_hWndOwner ) );
	}

	void CContextMenu::SetItem( PCIDLIST_ABSOLUTE pidlAbs )
	{
		Reset( shell::GetItemContextMenu( pidlAbs, m_hWndOwner ) );
	}

	void CContextMenu::SetItem( IShellItem* pItem )
	{
		Reset( shell::GetItemContextMenu( pItem, m_hWndOwner ) );
	}

	void CContextMenu::SetFilePath( const std::tstring& filePath )
	{
		Reset( shell::GetItemContextMenu( filePath, m_hWndOwner ) );
	}

	void CContextMenu::SetFilePaths( const std::vector< std::tstring >& filePaths )
	{
		Reset( shell::GetItemsContextMenu( filePaths, m_hWndOwner ) );
	}


	void CContextMenu::SetExternalPopupMenu( CMenu* pMenu )
	{
		if ( InternalMenu == m_menuOwnership )
			m_popupMenu.DestroyMenu();
		else
			m_popupMenu.Detach();

		if ( pMenu->GetSafeHmenu() != NULL )
		{
			m_popupMenu.Attach( pMenu->GetSafeHmenu() );
			m_menuOwnership = ExternalMenu;
		}
		else
		{
			m_popupMenu.CreatePopupMenu();
			m_menuOwnership = InternalMenu;
		}
	}

	bool CContextMenu::MakePopupMenu( CMenu& rPopupMenu, int atIndex /*= AtEnd*/, UINT queryFlags /*= CMF_NORMAL | CMF_EXPLORE*/ )
	{
		ASSERT( IsValid() );

		if ( NULL == rPopupMenu.GetSafeHmenu() )
			rPopupMenu.CreatePopupMenu();

		SetFlag( queryFlags, CMF_EXTENDEDVERBS, ui::IsKeyPressed( VK_SHIFT ) );
		if ( AtEnd == atIndex )
			atIndex = rPopupMenu.GetMenuItemCount();

		ui::CCmdIdStore oldIdsStore( rPopupMenu );			// commands belonging to the original popup menu, if initialized by the client

		if ( !HR_OK( m_pContextMenu->QueryContextMenu( rPopupMenu, atIndex, MinCmdId, MaxCmdId, queryFlags ) ) )
			return false;			// no common verbs for the given files

		m_shellIdStore.RegisterCommands( rPopupMenu );
		m_shellIdStore.Subtract( oldIdsStore );				// keep only the shell commmands
		return true;
	}

	int CContextMenu::TrackMenu( const CPoint& screenPos, UINT atIndex /*= AtEnd*/, UINT queryFlags /*= CMF_NORMAL | CMF_EXPLORE*/ )
	{
		if ( !IsValid() )
			return 0;						// no context menu

		CMenu* pPopupMenu = GetPopupMenu();
		if ( !MakePopupMenu( *pPopupMenu, atIndex, queryFlags ) )
			return 0;

		dbg::TraceMenu( pPopupMenu->GetSafeHmenu() );
		return TrackMenu( pPopupMenu, screenPos );
	}

	int CContextMenu::TrackMenu( CMenu* pPopupMenu, const CPoint& screenPos )
	{
		// subclass window to handle menu messages in CContextMenu
		ASSERT_NULL( m_pOldWndProc );
		if ( m_hWndOwner != NULL )
			if ( m_pContextMenu2 != NULL || m_pContextMenu3 != NULL )		// only subclass if its version 2 or 3
				m_pOldWndProc = (WNDPROC)::SetWindowLongPtr( m_hWndOwner, GWLP_WNDPROC, (LONG_PTR)HookWndProc );

		int cmdId = ui::ToCmdId( ::TrackPopupMenu( pPopupMenu->m_hMenu, TPM_RETURNCMD | TPM_LEFTALIGN, screenPos.x, screenPos.y, 0, m_hWndOwner, NULL ) );

		if ( m_pOldWndProc != NULL )		// unsubclass
		{
			::SetWindowLongPtr( m_hWndOwner, GWLP_WNDPROC, (LONG_PTR)m_pOldWndProc );
			m_pOldWndProc = NULL;
		}

		if ( cmdId >= MinCmdId && cmdId <= MaxCmdId )			// see if returned cmdId belongs to shell menu entries
		{
			InvokeVerbIndex( ToVerbIndex( cmdId ) );			// execute related command
			cmdId = 0;
		}
		return cmdId;
	}


	std::tstring CContextMenu::GetItemVerb( int cmdId ) const
	{
		ASSERT( IsValid() );

		TCHAR verb[ MAX_PATH ];
		if ( cmdId > 0 )
			if ( SUCCEEDED( m_pContextMenu->GetCommandString( ToVerbIndex( cmdId ), GCS_VERBW, NULL, (char*)verb, _countof( verb ) ) ) )
				return verb;

		return std::tstring();
	}

	bool CContextMenu::InvokeVerb( const char* pVerb )
	{
		ASSERT_PTR( pVerb );
		ASSERT( HasShellCmds() );			// context menu was queryed and there are common verbs on selected files

		CMINVOKECOMMANDINFO cmd;
		utl::ZeroWinStruct( &cmd );

		cmd.lpVerb = pVerb;
		cmd.hwnd = m_hWndOwner;
		cmd.nShow = SW_SHOWNORMAL;

		return HR_OK( m_pContextMenu->InvokeCommand( &cmd ) );
	}

	bool CContextMenu::InvokeDefaultVerb( void )
	{
		CMenu* pPopupMenu = EnsurePopupShellCmds( CMF_DEFAULTONLY );		// narrow down to default verb
		int defaultCmdId = ui::ToCmdId( pPopupMenu->GetDefaultItem( GMDI_GOINTOPOPUPS ) );

		return
			defaultCmdId >= MinCmdId &&
			InvokeVerbIndex( ToVerbIndex( defaultCmdId ) );
	}

	CMenu* CContextMenu::EnsurePopupShellCmds( UINT queryFlags )
	{
		if ( HasShellCmds() )
			return &m_popupMenu;

		// inplace query using a temporary popup menu
		CMenu* pPopupMenu = CMenu::FromHandle( ::CreatePopupMenu() );

		return MakePopupMenu( *pPopupMenu, AtEnd, queryFlags ) ? pPopupMenu : NULL;
	}

	LRESULT CContextMenu::HandleWndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
	{
		switch ( message )
		{
			case WM_MENUCHAR:		// only supported by IContextMenu3
				if ( m_pContextMenu3 != NULL )
				{
					LRESULT lResult = 0;
					m_pContextMenu3->HandleMenuMsg2( message, wParam, lParam, &lResult );
					return lResult;
				}
				break;
			case WM_DRAWITEM:
			case WM_MEASUREITEM:
				if ( wParam != 0 )
					break;			// message is not menu-related
				// fall-through
			case WM_INITMENUPOPUP:
				if ( m_pContextMenu2 != NULL )
					m_pContextMenu2->HandleMenuMsg( message, wParam, lParam );
				else	// version 3
					m_pContextMenu3->HandleMenuMsg( message, wParam, lParam );
				return message != WM_INITMENUPOPUP;		// inform caller that we handled WM_INITPOPUPMENU by ourself
		}

		return ::CallWindowProc( m_pOldWndProc, hWnd, message, wParam, lParam );		// call original WndProc
	}

	LRESULT CALLBACK CContextMenu::HookWndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
	{
		ASSERT_PTR( s_pInstance );
		return s_pInstance->HandleWndProc( hWnd, message, wParam, lParam );
	}


	// command handlers

	BEGIN_MESSAGE_MAP( CContextMenu, CCmdTarget )
	END_MESSAGE_MAP()

	BOOL CContextMenu::OnCmdMsg( UINT id, int code, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo )
	{
		return
			HasShellCmd( id ) &&
			__super::OnCmdMsg( id, code, pExtra, pHandlerInfo );		// shell commands are handled internally
	}
}
