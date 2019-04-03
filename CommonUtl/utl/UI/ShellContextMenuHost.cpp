
#include "stdafx.h"
#include "ShellContextMenuHost.h"
#include "ContainerUtilities.h"
#include "MenuUtilities.h"
#include "Utilities.h"
#include "WindowHook.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace shell
{
	CComPtr< IContextMenu > MakeAbsoluteContextMenu( PCIDLIST_ABSOLUTE pidlAbs, HWND hWndOwner )
	{
		CComPtr< IShellFolder > pParentFolder;
		PCUITEMID_CHILD pidlItem;			// no ownership (no allocation)
		::SHBindToParent( pidlAbs, IID_IShellFolder, (void**)&pParentFolder, &pidlItem );		// absolute pidl: get its parent folder and its relative PIDL

		return MakeFolderItemContextMenu( pParentFolder, pidlItem, hWndOwner );
	}

	CComPtr< IContextMenu > MakeFilePathContextMenu( const std::tstring& filePath, HWND hWndOwner )
	{
		CComHeapPtr< ITEMIDLIST_ABSOLUTE > pidlItem( static_cast< ITEMIDLIST_ABSOLUTE* >( ::ILCreateFromPath( filePath.c_str() ) ) );	// 64 bit: prevent warning C4090: 'argument' : different '__unaligned' qualifiers
		return MakeAbsoluteContextMenu( pidlItem, hWndOwner );
	}

	CComPtr< IContextMenu > MakeFolderItemContextMenu( IShellFolder* pParentFolder, PCITEMID_CHILD pidlItem, HWND hWndOwner )
	{
		ASSERT_PTR( pParentFolder );

		CComPtr< IContextMenu > pCtxMenu;
		HR_AUDIT( pParentFolder->GetUIObjectOf( hWndOwner, 1, &pidlItem, __uuidof( IContextMenu ), NULL, (void**)&pCtxMenu ) );
		return pCtxMenu;
	}

	CComPtr< IContextMenu > MakeItemContextMenu( IShellItem* pItem, HWND hWndOwner )
	{
		ASSERT_PTR( pItem );

		CComHeapPtr< ITEMID_CHILD > childPidl;
		if ( CComPtr< IShellFolder2 > pParentFolder = GetParentFolderAndPidl( &childPidl, pItem ) )
			return MakeFolderItemContextMenu( pParentFolder, childPidl, hWndOwner );

		return NULL;
	}

	CComPtr< IContextMenu > MakeFolderItemsContextMenu( IShellFolder* pParentFolder, PCUITEMID_CHILD_ARRAY pidlItemsArray, size_t itemCount, HWND hWndOwner )
	{
		ASSERT_PTR( pParentFolder );

		CComPtr< IContextMenu > pCtxMenu;
		HR_AUDIT( pParentFolder->GetUIObjectOf( hWndOwner, static_cast< unsigned int >( itemCount ), pidlItemsArray, __uuidof( IContextMenu ), NULL, (void**)&pCtxMenu ) );
		return pCtxMenu;
	}


	bool InvokeCommandByVerb( IContextMenu* pContextMenu, const char* pVerb, CWnd* pWndOwner )
	{	// hosts an IContextMenu and invokes a verb
		ASSERT_PTR( pContextMenu );

		CShellContextMenuHost ctxMenu( pWndOwner, pContextMenu );
		return
			ctxMenu.MakePopupMenu( CMF_VERBSONLY ) &&		// just verbs
			ctxMenu.InvokeVerb( pVerb );
	}

	bool InvokeDefaultVerb( IContextMenu* pContextMenu, CWnd* pWndOwner )
	{
		ASSERT_PTR( pContextMenu );

		CShellContextMenuHost ctxMenu( pWndOwner, pContextMenu );
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


// CShellContextMenuHost implementation

CShellContextMenuHost::CShellContextMenuHost( CWnd* pWndOwner, IContextMenu* pContextMenu /*= NULL*/ )
	: m_pWndOwner( pWndOwner )
	, m_menuOwnership( InternalMenu )
{
	m_popupMenu.CreatePopupMenu();
	Reset( pContextMenu );
}

CShellContextMenuHost::~CShellContextMenuHost()
{
	Reset();

	if ( ExternalMenu == m_menuOwnership )
		m_popupMenu.Detach();			// avoid destroying externally owned menu
}

void CShellContextMenuHost::Reset( IContextMenu* pContextMenu /*= NULL*/ )
{
	m_pContextMenu = pContextMenu;
}

void CShellContextMenuHost::SetPopupMenu( HMENU hMenu, MenuOwnership ownership /*= InternalMenu*/ )
{
	DeletePopupMenu();

	if ( hMenu != NULL )
	{
		m_popupMenu.Attach( hMenu );
		m_menuOwnership = ownership;
	}
	else
	{
		m_popupMenu.CreatePopupMenu();
		m_menuOwnership = InternalMenu;
	}
}

void CShellContextMenuHost::DeletePopupMenu( void )
{
	if ( InternalMenu == m_menuOwnership )
		m_popupMenu.DestroyMenu();
	else
		m_popupMenu.Detach();
}

bool CShellContextMenuHost::MakePopupMenu( CMenu& rPopupMenu, int atIndex /*= AtEnd*/, UINT queryFlags /*= CMF_EXPLORE*/ )
{
	if ( !IsValid() )
		return false;

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

int CShellContextMenuHost::TrackMenu( const CPoint& screenPos, UINT atIndex /*= AtEnd*/, UINT queryFlags /*= CMF_EXPLORE*/ )
{
	if ( !IsValid() )
		return 0;						// no context menu

	CMenu* pPopupMenu = GetPopupMenu();
	if ( !MakePopupMenu( *pPopupMenu, atIndex, queryFlags ) )
		return 0;

	return TrackMenu( pPopupMenu, screenPos );
}

int CShellContextMenuHost::TrackMenu( CMenu* pPopupMenu, const CPoint& screenPos, UINT trackFlags /*= TPM_RETURNCMD | TPM_RIGHTBUTTON*/ )
{
	CTrackingHook scopedHook( m_pWndOwner->GetSafeHwnd(), m_pContextMenu );
	return DoTrackMenu( pPopupMenu, screenPos, trackFlags );
}

int CShellContextMenuHost::DoTrackMenu( CMenu* pPopupMenu, const CPoint& screenPos, UINT trackFlags )
{
	//dbg::TraceMenu( pPopupMenu->GetSafeHmenu() );
	int cmdId = ui::TrackPopupMenu( *pPopupMenu, m_pWndOwner, screenPos, trackFlags );

	if ( HasFlag( trackFlags, TPM_RETURNCMD ) )			// handle command in-place?
		if ( HasShellCmd( cmdId ) )						// cmdId belongs to shell menu entries
		{
			OnShellCommand( cmdId );					// execute command
			return 0;									// handled internally
		}

	return cmdId;
}

bool CShellContextMenuHost::IsLazyUninit( void ) const
{
	return false;
}

CMenu* CShellContextMenuHost::EnsurePopupShellCmds( UINT queryFlags )
{
	if ( HasShellCmds() )
		return &m_popupMenu;

	// inplace query using a temporary popup menu
	CMenu* pPopupMenu = CMenu::FromHandle( ::CreatePopupMenu() );

	return MakePopupMenu( *pPopupMenu, AtEnd, queryFlags ) ? pPopupMenu : NULL;
}


std::tstring CShellContextMenuHost::GetItemVerb( int cmdId ) const
{
	ASSERT( IsValid() );

	TCHAR verb[ MAX_PATH ];
	if ( cmdId > 0 )
		if ( SUCCEEDED( m_pContextMenu->GetCommandString( ToVerbIndex( cmdId ), GCS_VERBW, NULL, (char*)verb, _countof( verb ) ) ) )
			return verb;

	return std::tstring();
}

bool CShellContextMenuHost::InvokeVerb( const char* pVerb )
{
	ASSERT_PTR( pVerb );
	// Note: allow verb invocation without checking ASSERT( HasShellCmds() ), since it could use a standard verb (no need to build the popup menu)

	CMINVOKECOMMANDINFO cmd;
	utl::ZeroWinStruct( &cmd );

	cmd.lpVerb = pVerb;
	cmd.hwnd = m_pWndOwner->GetSafeHwnd();
	cmd.nShow = SW_SHOWNORMAL;

	return HR_OK( m_pContextMenu->InvokeCommand( &cmd ) );
}

bool CShellContextMenuHost::InvokeVerbIndex( int verbIndex )
{
	ASSERT( verbIndex >= 0 );
	ASSERT( HasShellCmds() );			// context menu was queryed and there are common verbs on selected files

	return InvokeVerb( MAKEINTRESOURCEA( verbIndex ) );
}

bool CShellContextMenuHost::InvokeDefaultVerb( void )
{
	CMenu* pPopupMenu = EnsurePopupShellCmds( CMF_DEFAULTONLY );		// narrow down to default verb
	int defaultCmdId = ui::ToCmdId( pPopupMenu->GetDefaultItem( GMDI_GOINTOPOPUPS ) );

	return
		defaultCmdId >= MinCmdId &&
		InvokeVerbIndex( ToVerbIndex( defaultCmdId ) );
}


// command handlers

BEGIN_MESSAGE_MAP( CShellContextMenuHost, CCmdTarget )
	ON_COMMAND_RANGE( MinCmdId, MaxCmdId, OnShellCommand )
END_MESSAGE_MAP()

BOOL CShellContextMenuHost::OnCmdMsg( UINT id, int code, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo )
{
	return
		HasShellCmd( id ) &&
		__super::OnCmdMsg( id, code, pExtra, pHandlerInfo );		// shell commands are handled internally
}

void CShellContextMenuHost::OnShellCommand( UINT cmdId )
{
	ASSERT( HasShellCmd( cmdId ) );
	InvokeVerbIndex( ToVerbIndex( cmdId ) );	// execute related command
}


// CShellContextMenuHost::CTrackingHook implementation

CShellContextMenuHost::CTrackingHook* CShellContextMenuHost::CTrackingHook::s_pInstance = NULL;

CShellContextMenuHost::CTrackingHook::CTrackingHook( HWND hWndOwner, IContextMenu* pContextMenu )
	: m_hWndOwner( hWndOwner )
	, m_pOldWndProc( NULL )
	, m_pContextMenu2( pContextMenu )
	, m_pContextMenu3( pContextMenu )
{
	// only subclass if its version 2 or 3 of context menu - subclass window to handle menu messages in here
	if ( m_hWndOwner != NULL )
		if ( m_pContextMenu3 != NULL )
			m_pOldWndProc = (WNDPROC)::SetWindowLongPtr( m_hWndOwner, GWLP_WNDPROC, (LONG_PTR)HookWndProc3 );
		else if ( m_pContextMenu2 != NULL )
			m_pOldWndProc = (WNDPROC)::SetWindowLongPtr( m_hWndOwner, GWLP_WNDPROC, (LONG_PTR)HookWndProc2 );

	ASSERT_NULL( s_pInstance );
	s_pInstance = this;
}

CShellContextMenuHost::CTrackingHook::~CTrackingHook()
{
	if ( m_pOldWndProc != NULL )
		::SetWindowLongPtr( m_hWndOwner, GWLP_WNDPROC, (LONG_PTR)m_pOldWndProc );		// unsubclass

	ASSERT( this == s_pInstance );
	s_pInstance = NULL;
}

LRESULT CShellContextMenuHost::CTrackingHook::HandleWndProc2( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
	REQUIRE( m_pContextMenu2 != NULL && m_pContextMenu3 == NULL );

	switch ( message )
	{
		case WM_DRAWITEM:			// return value: if an application processes this message, it should return TRUE
		case WM_MEASUREITEM:		// return value: if an application processes this message, it should return TRUE
			if ( 0 == wParam )		// menu-related message?
				if ( SUCCEEDED( m_pContextMenu2->HandleMenuMsg( message, wParam, lParam ) ) )
					return TRUE;		// message was handled
			break;
		case WM_INITMENUPOPUP:		// return value: if an application processes this message, it should return 0
			if ( SUCCEEDED( m_pContextMenu2->HandleMenuMsg( message, wParam, lParam ) ) )
				return FALSE;			// message was handled
			break;
	}

	return ::CallWindowProc( m_pOldWndProc, hWnd, message, wParam, lParam );		// call original WndProc
}

LRESULT CShellContextMenuHost::CTrackingHook::HandleWndProc3( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
	REQUIRE( m_pContextMenu3 != NULL );

	LRESULT lResult = 0;

	switch ( message )
	{
		case WM_MENUCHAR:			// handled only by IContextMenu3
			if ( SUCCEEDED( m_pContextMenu3->HandleMenuMsg2( message, wParam, lParam, &lResult ) ) )
				return lResult;					// return value is important!
			break;
		case WM_DRAWITEM:			// return value: if an application processes this message, it should return TRUE
		case WM_MEASUREITEM:
			if ( 0 == wParam )					// menu-related message?
				if ( SUCCEEDED( m_pContextMenu3->HandleMenuMsg2( message, wParam, lParam, &lResult ) ) )
					if ( TRUE == lResult )		// message was handled?
						return lResult;
			break;
		case WM_INITMENUPOPUP:		// return value: if an application processes this message, it should return 0
			if ( SUCCEEDED( m_pContextMenu3->HandleMenuMsg2( message, wParam, lParam, &lResult ) ) )
				if ( 0 == lResult )				// message was handled?
					return lResult;
	}

	return ::CallWindowProc( m_pOldWndProc, hWnd, message, wParam, lParam );		// call original WndProc
}

LRESULT CALLBACK CShellContextMenuHost::CTrackingHook::HookWndProc2( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
	ASSERT_PTR( s_pInstance );
	return s_pInstance->HandleWndProc2( hWnd, message, wParam, lParam );
}

LRESULT CALLBACK CShellContextMenuHost::CTrackingHook::HookWndProc3( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
	ASSERT_PTR( s_pInstance );
	return s_pInstance->HandleWndProc3( hWnd, message, wParam, lParam );
}


// abstract base for short-lived dynamic objects that execute a delayed method call

class CExplorerSubMenuHook : public CWindowHook
{
public:
	CExplorerSubMenuHook( CShellLazyContextMenuHost* pLazyHost, CWnd* pWndOwner ) : CWindowHook( false ), m_pLazyHost( pLazyHost ) { HookWindow( pWndOwner->GetSafeHwnd() ); }
	virtual ~CExplorerSubMenuHook() { if ( IsHooked() ) UnhookWindow(); }
protected:
	virtual LRESULT WindowProc( UINT message, WPARAM wParam, LPARAM lParam );		// base override
private:
	CShellLazyContextMenuHost* m_pLazyHost;
};


LRESULT CExplorerSubMenuHook::WindowProc( UINT message, WPARAM wParam, LPARAM lParam )
{
	if ( WM_INITMENUPOPUP == message )
		if ( (HMENU)wParam == m_pLazyHost->GetPopupMenu()->GetSafeHmenu() )
		{
			UnhookWindow();
			m_pLazyHost->LazyInit();		// will delete this!
			return 0L;
		}

	return CWindowHook::WindowProc( message, wParam, lParam );
}


// CShellLazyContextMenuHost implementation

CShellLazyContextMenuHost::CShellLazyContextMenuHost( CWnd* pWndOwner, const std::vector< std::tstring >& filePaths, UINT queryFlags /*= CMF_NORMAL*/ )
	: CShellContextMenuHost( pWndOwner )
	, m_filePaths( filePaths )
	, m_queryFlags( queryFlags )
	, m_isLazyInit( false )
{
	SetPopupMenu( ::CreatePopupMenu(), CShellContextMenuHost::ExternalMenu );		// will be owned by the menu having the "Explorer" sub-menu
}

CShellLazyContextMenuHost::~CShellLazyContextMenuHost()
{
}

int CShellLazyContextMenuHost::TrackMenu( CMenu* pPopupMenu, const CPoint& screenPos, UINT trackFlags /*= TPM_RETURNCMD | TPM_RIGHTBUTTON*/ )
{
	if ( !IsLazyUninit() )
		return __super::TrackMenu( pPopupMenu, screenPos, trackFlags );		// track directly for subsequent calls
	else if ( NULL == m_pExplorerSubMenuHook.get() )
		m_pExplorerSubMenuHook.reset( new CExplorerSubMenuHook( this, m_pWndOwner ) );

	int cmdId = DoTrackMenu( pPopupMenu, screenPos, trackFlags );			// no CTrackingHook yet!

	m_pTrackingHook.reset();
	return cmdId;
}

bool CShellLazyContextMenuHost::IsLazyUninit( void ) const
{
	return !m_isLazyInit;
}

bool CShellLazyContextMenuHost::LazyInit( void )
{
	ASSERT( !m_isLazyInit );		// call once
	m_isLazyInit = true;

	m_pExplorerSubMenuHook.reset();

	TRACE( _T(" (!) LazyInit() on Explorer sub-menu.\n") );
	CWaitCursor wait;
	if ( CComPtr< IContextMenu > pContextMenu = shell::MakeFilePathsContextMenu( m_filePaths, m_pWndOwner->GetSafeHwnd() ) )
	{
		Reset( pContextMenu );

		if ( MakePopupMenu( m_queryFlags ) )
			m_pTrackingHook.reset( new CTrackingHook( m_pWndOwner->GetSafeHwnd(), Get() ) );

		return true;
	}

	ui::BeepSignal( MB_ICONWARNING );
	return false;
}
