
#include "pch.h"
#include "BaseMainDialog.h"
#include "BaseApp.h"
#include "Icon.h"
#include "SystemTray.h"
#include "MenuUtilities.h"
#include "StringUtilities.h"
#include "WndUtils.h"
#include "PostCall.h"
#include "resource.h"
#include <afxwinappex.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CBaseMainDialog::CBaseMainDialog( UINT templateId, CWnd* pParent /*= nullptr*/ )
	: CLayoutDialog( templateId, pParent )
{
	m_initCentered = false;			// so that it uses WINDOWPLACEMENT
	SetTopDlg();
	LoadDlgIcon( templateId );		// needs both small/large icons for main dialog (task switching needs the large one)
}

CBaseMainDialog::~CBaseMainDialog()
{
	if ( this == AfxGetMainWnd() )
		AfxGetApp()->m_pMainWnd = nullptr;
}

void CBaseMainDialog::ParseCommandLine( int argc, TCHAR* argv[] )
{
	std::vector<std::tstring> unknown;

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

void CBaseMainDialog::PostRestorePlacement( int showCmd, bool restoreToMaximized ) override
{
	if ( UseSysTrayMinimize() )
		if ( SW_SHOWMINIMIZED == showCmd )
		{
			// IMP: for some reason in Windows 10 WM_SYSCOMMAND message doesn't post properly, but CBasePostCall::WM_DELAYED_CALL posts fine
			ui::PostCall( this, &CBaseMainDialog::_Minimize, restoreToMaximized );
			//PostMessage( WM_SYSCOMMAND, SC_MINIMIZE );
		}
		else if ( -1 == showCmd )			// use command line show option
			switch ( AfxGetApp()->m_nCmdShow )
			{
				case SW_SHOWMINNOACTIVE:
				case SW_SHOWMINIMIZED:
				case SW_MINIMIZE:
					ModifyStyle( 0, WS_VISIBLE );			// prevents modal dialog loop to show this window, without actually showing the window (no startup flicker)
					SendMessage( WM_SYSCOMMAND, SC_MINIMIZE );
					break;
			}
}

CWnd* CBaseMainDialog::GetOwnerWnd( void ) override
{
	return this;
}

CMenu* CBaseMainDialog::GetTrayIconContextMenu( void ) override
{
	return UseSysTrayMinimize() ? &m_trayPopupMenu : nullptr;
}

bool CBaseMainDialog::OnTrayIconNotify( UINT msgNotifyCode, UINT trayIconId, const CPoint& screenPos ) override
{
	msgNotifyCode, trayIconId, screenPos;
	return false;
}

void CBaseMainDialog::_Minimize( bool restoreToMaximized )
{
	ASSERT_PTR( m_pSystemTray.get() );
	m_pSystemTray->MinimizeOwnerWnd( restoreToMaximized );
}


// message handlers

BEGIN_MESSAGE_MAP( CBaseMainDialog, CLayoutDialog )
	ON_WM_CONTEXTMENU()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_COMMAND( ID_APP_RESTORE, OnAppRestore )
	ON_UPDATE_COMMAND_UI( ID_APP_RESTORE, OnUpdateAppRestore )
	ON_COMMAND( ID_APP_MINIMIZE, OnAppMinimize )
	ON_UPDATE_COMMAND_UI( ID_APP_MINIMIZE, OnUpdateAppMinimize )
END_MESSAGE_MAP()

BOOL CBaseMainDialog::OnInitDialog( void ) override
{
	CWinApp* pApp = AfxGetApp();
	const std::tstring& appNameSuffix = is_a<CWinAppEx>( pApp )
		? checked_static_cast< CBaseApp<CWinAppEx>* >( pApp )->GetAppNameSuffix()
		: checked_static_cast< CBaseApp<CWinApp>* >( pApp )->GetAppNameSuffix();

	if ( !appNameSuffix.empty() )
		ui::SetWindowText( m_hWnd, ui::GetWindowText( m_hWnd ) + appNameSuffix );

	if ( nullptr == AfxGetMainWnd() )
		AfxGetApp()->m_pMainWnd = this;

	if ( HasFlag( GetExStyle(), WS_EX_TOOLWINDOW ) )
		ModifyStyleEx( WS_EX_APPWINDOW, 0 );		// prevent excluding this application from taskbar

	if ( UseSysTrayMinimize() )
	{	// add the shell tray icon
		m_pSystemTray.reset( new CSystemTrayWndHook() );
		m_pSystemTray->SetOwnerCallback( this );
		m_pSystemTray->CreateTrayIcon( GetDlgIcon( DlgSmallIcon )->GetSafeHandle(), ShellIconId, ui::GetWindowText( this ).c_str() );
	}

	return __super::OnInitDialog();
}

void CBaseMainDialog::OnContextMenu( CWnd* pWnd, CPoint screenPos )
{
	if ( this == pWnd )
	{
		app::TrackUnitTestMenu( this, screenPos );
		return;
	}

	__super::OnContextMenu( pWnd, screenPos );
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
		__super::OnPaint();
}

HCURSOR CBaseMainDialog::OnQueryDragIcon( void )
{	// the system calls this function to obtain the cursor to display while the user drags the minimized window
	if ( const CIcon* pSmallIcon = GetDlgIcon( DlgSmallIcon ) )
		return pSmallIcon->GetHandle();
	return nullptr;
}

void CBaseMainDialog::OnAppRestore( void )
{
	SendMessage( WM_SYSCOMMAND, SC_RESTORE );
}

void CBaseMainDialog::OnUpdateAppRestore( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( CSystemTray::IsMinimizedToTray( this ) );
}

void CBaseMainDialog::OnAppMinimize( void )
{
	SendMessage( WM_SYSCOMMAND, SC_MINIMIZE );
}

void CBaseMainDialog::OnUpdateAppMinimize( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( !CSystemTray::IsMinimizedToTray( AfxGetMainWnd() ) );
}
