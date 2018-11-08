
#include "stdafx.h"
#include "ShellContextMenu.h"
#include "ContainerUtilities.h"
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


	HRESULT InvokeCommandByVerb( IContextMenu* pContextMenu, const char* pVerb, HWND hWnd )
	{	// hosts an IContextMenu and invokes a verb
		ASSERT_PTR( pContextMenu );

		CMenu popupMenu;
		popupMenu.CreatePopupMenu();		// temporary popup menu

		HRESULT hr = pContextMenu->QueryContextMenu( popupMenu, 0, 1, 0x7FFF, CMF_NORMAL );
		if ( SUCCEEDED( hr ) )				// is there a common context menu for the selected files?
		{
			CMINVOKECOMMANDINFO cmd;
			utl::ZeroWinStruct( &cmd );

			cmd.hwnd = hWnd;
			cmd.lpVerb = pVerb;
			cmd.nShow = SW_SHOWNORMAL;

			return pContextMenu->InvokeCommand( &cmd );
		}
		return hr;
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

//	bool ExecItemVerb( IShellFolder* pParentFolder, const ITEMID_CHILD* pidl, const TCHAR* pVerb, HWND hWndOwner, DWORD mask /*= 0*/, int show /*= SW_SHOWNORMAL*/ )
//	{
//		ASSERT_PTR( pVerb );
//
//		CContextMenu contextMenu;
//		if ( contextMenu.SetFromItem( pParentFolder, pidl, hWndOwner ) )
//		{
//			int cmdId = contextMenu.FindCmdIdWithVerb( pVerb );
//			if ( cmdId > 0 )
//				return contextMenu.InvokeCommand( cmdId, hWndOwner, mask, show );
//		}
//
//		return false;
//	}
//
//	bool ExecItemVerb( IShellItem* pItem, const TCHAR* pVerb, HWND hWndOwner, DWORD mask /*= 0*/, int show /*= SW_SHOWNORMAL*/ )
//	{
//		ASSERT_PTR( pVerb );
//
//		CContextMenu contextMenu;
//		if ( contextMenu.SetFromItem( pItem, hWndOwner ) )
//		{
//			int cmdId = contextMenu.FindCmdIdWithVerb( pVerb );
//			if ( cmdId > 0 )
//				return contextMenu.InvokeCommand( cmdId, hWndOwner, mask, show );
//		}
//
//		return false;
//	}

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


	bool CContextMenu::MakePopupMenu( CMenu& rPopupMenu, UINT atIndex /*= AtEnd*/, UINT flags /*= CMF_NORMAL | CMF_EXPLORE*/, UINT cmdIdFirst /*= MinCmdId*/, UINT cmdIdLast /*= MaxCmdId*/ ) const
	{
		ASSERT( IsValid() );

		if ( NULL == rPopupMenu.GetSafeHmenu() )
			rPopupMenu.CreatePopupMenu();

		SetFlag( flags, CMF_EXTENDEDVERBS, ui::IsKeyPressed( VK_SHIFT ) );
		if ( AtEnd == atIndex )
			atIndex = rPopupMenu.GetMenuItemCount();

		return HR_OK( m_pContextMenu->QueryContextMenu( rPopupMenu, atIndex, cmdIdFirst, cmdIdLast, flags ) );
	}

	int CContextMenu::TrackMenu( const CPoint& screenPos, UINT atIndex /*= AtEnd*/, UINT flags /*= CMF_NORMAL | CMF_EXPLORE*/ )
	{
		if ( !IsValid() )
			return 0;						// no context menu

		CMenu* pPopupMenu = GetPopupMenu();
		if ( !MakePopupMenu( *pPopupMenu, atIndex, flags ) )
			return 0;

		return TrackMenu( pPopupMenu, screenPos );
	}

	int CContextMenu::TrackMenu( CMenu* pPopupMenu, const CPoint& screenPos )
	{
		// subclass window to handle menu messages in CContextMenu
		ASSERT_NULL( m_pOldWndProc );
		if ( m_hWndOwner != NULL )
			if ( m_pContextMenu2 != NULL || m_pContextMenu3 != NULL )		// only subclass if its version 2 or 3
				m_pOldWndProc = (WNDPROC)::SetWindowLongPtr( m_hWndOwner, GWLP_WNDPROC, (LONG_PTR)HookWndProc );

		UINT cmdId = ::TrackPopupMenu( pPopupMenu->m_hMenu, TPM_RETURNCMD | TPM_LEFTALIGN, screenPos.x, screenPos.y, 0, m_hWndOwner, NULL );

		if ( m_pOldWndProc != NULL )		// unsubclass
		{
			::SetWindowLongPtr( m_hWndOwner, GWLP_WNDPROC, (LONG_PTR)m_pOldWndProc );
			m_pOldWndProc = NULL;
		}

		if ( cmdId >= MinCmdId && cmdId <= MaxCmdId )	// see if returned cmdId belongs to shell menu entries
		{
			InvokeCommand( cmdId - MinCmdId );	// execute related command
			cmdId = 0;
		}
		return cmdId;
	}

	bool CContextMenu::InvokeCommand( UINT cmdId )
	{
		CMINVOKECOMMANDINFO cmd;
		utl::ZeroWinStruct( &cmd );

		cmd.lpVerb = MAKEINTRESOURCEA( cmdId );
		cmd.hwnd = m_hWndOwner;
		cmd.nShow = SW_SHOWNORMAL;

		return HR_OK( m_pContextMenu->InvokeCommand( &cmd ) );
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
}
