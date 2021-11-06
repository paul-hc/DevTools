
#include "stdafx.h"
#include "BaseMainDialog.h"
#include "BaseApp.h"
#include "Icon.h"
#include "MenuUtilities.h"
#include "StringUtilities.h"
#include "Utilities.h"
#include "PostCall.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


const UINT CBaseMainDialog::WM_TASKBARCREATED = ::RegisterWindowMessageA( "TaskbarCreated" );
const UINT CBaseMainDialog::WM_TRAYICONNOTIFY = ::RegisterWindowMessageA( "utl:WM_TRAYICONNOTIFY" );


CBaseMainDialog::CBaseMainDialog( UINT templateId, CWnd* pParent /*= NULL*/ )
	: CLayoutDialog( templateId, pParent )
{
	m_initCentered = false;			// so that it uses WINDOWPLACEMENT
	SetTopDlg();
	LoadDlgIcon( templateId );		// needs both small/large icons for main dialog (task switching needs the large one)
}

CBaseMainDialog::~CBaseMainDialog()
{
	if ( this == AfxGetMainWnd() )
		AfxGetApp()->m_pMainWnd = NULL;
}

void CBaseMainDialog::ParseCommandLine( int argc, TCHAR* argv[] )
{
	std::vector< std::tstring > unknown;

	for ( int i = 1; i < argc; ++i )
		if ( const TCHAR* pSwitch = arg::GetSwitch( argv[ i ] ) )
			if ( arg::Equals( pSwitch, _T("max") ) )
				AfxGetApp()->m_nCmdShow = SW_SHOWMAXIMIZED;
			else if ( arg::Equals( pSwitch, _T("min") ) )
				AfxGetApp()->m_nCmdShow = SW_SHOWMINNOACTIVE;
}

void CBaseMainDialog::ShowAll( bool show )
{
	if ( HWND hMainWnd = AfxGetMainWnd()->GetSafeHwnd() )
	{
		::ShowWindow( hMainWnd, show ? SW_SHOW : SW_HIDE );
		::ShowOwnedPopups( hMainWnd, show );
	}
}

void CBaseMainDialog::PostRestorePlacement( int showCmd )
{
	if ( UseSysTrayMinimize() )
		if ( SW_SHOWMINIMIZED == showCmd )
		{
			// IMP: for some reason in Windows 10 WM_SYSCOMMAND message doesn't post properly, but CBasePostCall::WM_DELAYED_CALL posts fine
			ui::PostCall( this, &CBaseMainDialog::_Minimize );
			//PostMessage( WM_SYSCOMMAND, SC_MINIMIZE );
		}
		else if ( -1 == showCmd )			// use command line show option
			switch ( AfxGetApp()->m_nCmdShow )
			{
				case SW_SHOWMINNOACTIVE:
				case SW_SHOWMINIMIZED:
				case SW_MINIMIZE:
					ModifyStyle( 0, WS_VISIBLE );			// prevents modal dialog loop to show this window, whithout actually showing the window (no startup flicker)
					SendMessage( WM_SYSCOMMAND, SC_MINIMIZE );
					break;
			}
}

void CBaseMainDialog::_Minimize( void )
{
	SendMessage( WM_SYSCOMMAND, SC_MINIMIZE );
}

bool CBaseMainDialog::NotifyTrayIcon( int notifyCode )
{
	NOTIFYICONDATA niData = { sizeof( NOTIFYICONDATA ) };

	niData.hWnd = m_hWnd;
	niData.uID = ShellIconId;
	niData.uFlags = NIF_ICON;
	niData.uCallbackMessage = WM_TRAYICONNOTIFY;
	niData.hIcon = NULL;
	niData.szTip[ 0 ] = _T('\0');
	niData.uVersion = NOTIFYICON_VERSION;

	if ( NIM_ADD == notifyCode || NIM_MODIFY == notifyCode )
	{
		niData.uFlags |= NIF_MESSAGE | NIF_TIP;
		niData.hIcon = GetDlgIcon( DlgSmallIcon )->GetHandle();
		GetWindowText( niData.szTip, COUNT_OF( niData.szTip ) );
	}
	return ::Shell_NotifyIcon( notifyCode, &niData ) != FALSE;
}


// message handlers

BEGIN_MESSAGE_MAP( CBaseMainDialog, CLayoutDialog )
	ON_WM_DESTROY()
	ON_WM_CONTEXTMENU()
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_REGISTERED_MESSAGE( WM_TRAYICONNOTIFY, OnTrayIconNotify )
	ON_REGISTERED_MESSAGE( WM_TASKBARCREATED, OnExplorerRestart )
END_MESSAGE_MAP()

BOOL CBaseMainDialog::OnInitDialog( void )
{
	const std::tstring& appNameSuffix = checked_static_cast< CBaseApp<CWinApp>* >( AfxGetApp() )->GetAppNameSuffix();
	if ( !appNameSuffix.empty() )
		ui::SetWindowText( m_hWnd, ui::GetWindowText( m_hWnd ) + appNameSuffix );

	if ( NULL == AfxGetMainWnd() )
		AfxGetApp()->m_pMainWnd = this;

	if ( HasFlag( GetExStyle(), WS_EX_TOOLWINDOW ) )
		ModifyStyleEx( WS_EX_APPWINDOW, 0 );		// prevent excluding this application from taskbar

	if ( UseSysTrayMinimize() )
		NotifyTrayIcon( NIM_ADD );					// add shell tray icon

	return CLayoutDialog::OnInitDialog();
}

void CBaseMainDialog::OnDestroy( void )
{
	NotifyTrayIcon( NIM_DELETE );					// remove shell tray icon
	CLayoutDialog::OnDestroy();
}

void CBaseMainDialog::OnContextMenu( CWnd* pWnd, CPoint screenPos )
{
	if ( this == pWnd )
	{
		app::TrackUnitTestMenu( this, screenPos );
		return;
	}

	CLayoutDialog::OnContextMenu( pWnd, screenPos );
}

void CBaseMainDialog::OnSysCommand( UINT cmdId, LPARAM lParam )
{
	CLayoutDialog::OnSysCommand( cmdId, lParam );

	if ( SC_MINIMIZE == GET_SC_WPARAM( cmdId ) )
		if ( UseSysTrayMinimize() )
			ShowWindow( SW_HIDE );			// IMP: hide window post-minimize so that it vanishes from the taskbar into the system tray
}

void CBaseMainDialog::OnPaint( void )
{
	if ( IsIconic() )
	{
		CPaintDC dc( this );		// device context for painting
		SendMessage( WM_ICONERASEBKGND, reinterpret_cast<WPARAM>( dc.GetSafeHdc() ), 0 );

		if ( const CIcon* pSmallIcon = GetDlgIcon( DlgSmallIcon ) )
		{
			// center icon in client rectangle
			int cxIcon = GetSystemMetrics( SM_CXICON ), cyIcon = GetSystemMetrics( SM_CYICON );
			CRect rect;
			GetClientRect( &rect );

			int x = ( rect.Width() - cxIcon + 1 ) / 2, y = ( rect.Height() - cyIcon + 1 ) / 2;
			dc.DrawIcon( x, y, pSmallIcon->GetHandle() );
		}
	}
	else
		CLayoutDialog::OnPaint();
}

HCURSOR CBaseMainDialog::OnQueryDragIcon( void )
{	// the system calls this function to obtain the cursor to display while the user drags the minimized window
	if ( const CIcon* pSmallIcon = GetDlgIcon( DlgSmallIcon ) )
		return pSmallIcon->GetHandle();
	return NULL;
}

LRESULT CBaseMainDialog::OnTrayIconNotify( WPARAM wParam, LPARAM lParam )
{
	if ( ShellIconId == wParam )
		switch ( lParam )
		{
			case WM_LBUTTONDOWN:
				SendMessage( WM_SYSCOMMAND, SC_RESTORE );
				SetForegroundWindow();
				break;
			case WM_RBUTTONUP:
				if ( m_pSystemTrayInfo->m_popupMenu.GetSafeHmenu() != NULL )
					ui::TrackPopupMenu( m_pSystemTrayInfo->m_popupMenu, this, ui::GetCursorPos(), TPM_RIGHTBUTTON );
				break;
		}

	return 0L;
}

// this is called whenever the taskbar is created, e.g. after Explorer crashes and restarts.
LRESULT CBaseMainDialog::OnExplorerRestart( WPARAM, LPARAM )
{
	NotifyTrayIcon( NIM_ADD );					// add shell tray icon
	return 0L;
}
