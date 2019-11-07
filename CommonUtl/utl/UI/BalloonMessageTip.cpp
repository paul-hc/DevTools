
#include "stdafx.h"
#include "BalloonMessageTip.h"
#include "CmdInfoStore.h"
#include "Utilities.h"
#include "utl/ContainerUtilities.h"
#include "utl/EnumTags.h"
#include "utl/Registry.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace reg
{
	static const TCHAR section_explorerAdvanced[] = _T("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced");
	static const TCHAR entry_enableBalloonTips[] = _T("EnableBalloonTips");
}


namespace ui
{
	bool BalloonsEnabled( void )
	{
		// Verify that balloons have not been suppressed via a registry tweak.
		//
		// Thanks to DerMeister at CodeProject for spotting this:
		// http://www.codeproject.com/script/Membership/Profiles.aspx?mid=22871
		reg::CKey key;
		if ( key.Open( HKEY_CURRENT_USER, reg::section_explorerAdvanced, KEY_READ ) )
			return key.ReadNumberValue< DWORD >( reg::entry_enableBalloonTips, TRUE ) != FALSE;

		return true;
	}

	void RequestCloseAllBalloons( void )
	{
		CBalloonUiThread::RequestCloseAll();
	}

	const CPoint& GetNullPos( void )
	{
		static const CPoint s_nullPosition( INT_MAX, INT_MAX );
		return s_nullPosition;
	}

	CPoint GetTipScreenPos( const CWnd* pCtrl )
	{
		if ( pCtrl->GetSafeHwnd() != NULL )
		{
			CRect windowRect;
			pCtrl->GetWindowRect( &windowRect );

			if ( HasFlag( pCtrl->GetStyle(), WS_DLGFRAME ) )		// has a caption (top-level or child)?
			{
				CRect clientRect;
				pCtrl->GetClientRect( &clientRect );
				ui::ClientToScreen( pCtrl->GetSafeHwnd(), clientRect );

				if ( pCtrl->GetMenu() != NULL )
				{
					MENUBARINFO menuBarInfo;
					utl::ZeroWinStruct( &menuBarInfo );
					if ( ::GetMenuBarInfo( pCtrl->m_hWnd, OBJID_MENU, 0, &menuBarInfo ) )
						clientRect.top -= ( menuBarInfo.rcBar.bottom - menuBarInfo.rcBar.top );			// subtract the menu bar area to center in the caption bar
				}

				windowRect.bottom = clientRect.top;		// center on the top caption area
			}

			return windowRect.CenterPoint();			// balloon will be centered in the ctrl area
		}

		return GetNullPos();
	}


	CBalloonHostWnd* ShowBalloonTip( const TCHAR* pTitle, const std::tstring& message, HICON hToolIcon /*= TTI_NONE*/, const CPoint& screenPos /*= GetNullPos()*/ )
	{
		return CBalloonHostWnd::Display( pTitle, message, hToolIcon, screenPos );
	}

	CBalloonHostWnd* ShowBalloonTip( const CWnd* pCtrl, const TCHAR* pTitle, const std::tstring& message, HICON hToolIcon /*= TTI_NONE*/ )
	{
		return ShowBalloonTip( pTitle, message, hToolIcon, GetTipScreenPos( pCtrl ) );
	}

	CBalloonHostWnd* SafeShowBalloonTip( UINT mbStyle, const TCHAR* pTitle, const std::tstring& message, CWnd* pCtrl )
	{
		// these styles are inappropriate for Balloon presentation:
		ASSERT( !HasFlag( mbStyle, MB_ABORTRETRYIGNORE | MB_OKCANCEL | MB_RETRYCANCEL | MB_YESNO | MB_YESNOCANCEL ) );
		ASSERT( ( mbStyle & (MB_ICONINFORMATION | MB_ICONEXCLAMATION | MB_ICONSTOP | MB_ICONQUESTION) ) != MB_ICONQUESTION );

		if ( NULL == pCtrl )
			pCtrl = CWnd::GetSafeOwner();

		if ( BalloonsEnabled() && pCtrl != NULL )			// user hasn't disabled tooltip display in Explorer?
		{
			HICON hToolIcon = TTI_NONE;

			if ( HasFlag( mbStyle, MB_ICONINFORMATION ) )
				hToolIcon = (HICON)TTI_INFO;
			else if ( HasFlag( mbStyle, MB_ICONEXCLAMATION ) )
				hToolIcon = (HICON)TTI_WARNING;
			else if ( HasFlag( mbStyle, MB_ICONSTOP ) )
				hToolIcon = (HICON)TTI_ERROR;

			return ShowBalloonTip( pCtrl, pTitle, message, hToolIcon );
		}

		ui::MessageBox( message, mbStyle );				// default handling when user has disabled balloons
		return NULL;
	}
}


// CGuiThreadInfo implementation

CGuiThreadInfo::CGuiThreadInfo( DWORD threadId /*= 0*/ )
{
	utl::ZeroWinStruct( static_cast< tagGUITHREADINFO* >( this ) );

	if ( threadId != 0 )
		ReadInfo( threadId );
}

void CGuiThreadInfo::ReadInfo( DWORD threadId )
{
	utl::ZeroWinStruct( static_cast< tagGUITHREADINFO* >( this ) );
	VERIFY( ::GetGUIThreadInfo( threadId, this ) );
}

CGuiThreadInfo::Change CGuiThreadInfo::Compare( const GUITHREADINFO& right ) const
{
	if ( hwndFocus != right.hwndFocus || hwndActive != right.hwndActive )
		return FocusChanged;		// good indication that the user is doing something new

	if ( hwndCapture != right.hwndCapture )
		return CaptureChanged;		// another good indication that the user is doing something new

	if ( ( flags & ~GUI_CARETBLINKING ) != ( right.flags & ~GUI_CARETBLINKING ) )
		return FlagsChanged;		// other state changes can be initiated by keys, etc

	return None;
}

const CEnumTags& CGuiThreadInfo::GetTags_Change( void )
{
	static const CEnumTags tags( _T("None|Focus/Activation|Capture|Flags") );
	return tags;
}


// CBalloonHostWnd implementation

UINT CBalloonHostWnd::s_autoPopTimeout = AutoPopTimeout;
UINT CBalloonHostWnd::s_maxTipWidth = MaxTipWidth;
UINT CBalloonHostWnd::s_toolBorder = ToolBorder;

CBalloonHostWnd::CBalloonHostWnd( void )
	: CWnd()
	, m_mainThreadId( ::GetCurrentThreadId() )		// assume that this is created from the main calling thread
	, m_hToolIcon( TTI_NONE )
	, m_screenPos( ui::GetNullPos() )
	, m_autoPopTimeout( s_autoPopTimeout )
	, m_timer( this, TimerEventId, EllapseMs )
	, m_eventCount( 0 )
{
}

CBalloonHostWnd::~CBalloonHostWnd()
{
	if ( m_hWnd != NULL )
		DestroyWindow();
}

CBalloonHostWnd* CBalloonHostWnd::Display( const TCHAR* pTitle, const std::tstring& message, HICON hToolIcon /*= TTI_NONE*/, const CPoint& screenPos /*= ui::GetNullPos()*/ )
{
	// create the thread and initialise the thread and its transparent balloon parent window
	CBalloonUiThread* pNewThread = checked_static_cast< CBalloonUiThread* >( ::AfxBeginThread( RUNTIME_CLASS( CBalloonUiThread ), THREAD_PRIORITY_NORMAL, 0, CREATE_SUSPENDED ) );
	CBalloonHostWnd* pHostWnd = pNewThread->GetHostWnd();

	pHostWnd->m_title = pTitle;
	pHostWnd->m_message = message;
	pHostWnd->m_hToolIcon = hToolIcon;
	pHostWnd->m_screenPos = screenPos;

	pNewThread->ResumeThread();
	return pHostWnd;
}

bool CBalloonHostWnd::Create( CWnd* pParentWnd )
{
	static const CBrush s_debugBk( color::Red );				// to help visualize the host window
	static const std::tstring s_wndClass = ::AfxRegisterWndClass( 0, NULL, NULL /*s_debugBk*/ );		// register class with all defaults
	CRect mouseRect( m_screenPos != ui::GetNullPos() ? m_screenPos : ui::GetCursorPos(), CSize( 0, 0 ) );

	mouseRect.InflateRect( s_toolBorder, s_toolBorder );

	// create the transparent host window
	return CreateEx( WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW,
					 s_wndClass.c_str(),
					 _T("<balloon_host>"),
					 WS_POPUP,
					 mouseRect.left, mouseRect.top,
					 mouseRect.Width(), mouseRect.Height(),
					 pParentWnd->GetSafeHwnd(),
					 NULL ) != FALSE;
}

bool CBalloonHostWnd::CreateToolTip( void )
{
	if ( !m_toolTipCtrl.Create( this, TTS_ALWAYSTIP | TTS_NOPREFIX | TTS_BALLOON ) )
		return false;

	TOOLINFO toolInfo;
	utl::ZeroWinStruct( &toolInfo );

	// add a tool that covers this whole window
	toolInfo.uFlags = TTF_TRACK;
	toolInfo.hwnd = m_hWnd;
	toolInfo.lpszText = const_cast< TCHAR* >( m_message.c_str() ); //LPSTR_TEXTCALLBACK;
	toolInfo.uId = ToolId;
	GetClientRect( &toolInfo.rect );

	if ( !m_toolTipCtrl.SendMessage( TTM_ADDTOOL, 0, (LPARAM)(LPTOOLINFO)&toolInfo ) )
		return false;

	m_toolTipCtrl.SetTitle( (UINT)(UINT_PTR)m_hToolIcon, m_title.c_str() );					// store the icon and title

	if ( m_screenPos != ui::GetNullPos() )
		m_toolTipCtrl.SendMessage( TTM_TRACKPOSITION, 0, (LPARAM)MAKELPARAM( m_screenPos.x, m_screenPos.y ) );

	m_toolTipCtrl.SendMessage( TTM_TRACKACTIVATE, (WPARAM)TRUE, (LPARAM)&toolInfo );		// activate the tracking tooltip
	return true;
}

void CBalloonHostWnd::UpdateTitle( const TCHAR* pTitle, HICON hToolIcon /*= TTI_NONE*/ )
{
	m_title = pTitle;
	m_hToolIcon = hToolIcon;

	m_toolTipCtrl.SetTitle( (UINT)(UINT_PTR)m_hToolIcon, m_title.c_str() );					// store the icon and title
}

void CBalloonHostWnd::UpdateMessage( const std::tstring& message )
{
	m_message = message;

	m_toolTipCtrl.UpdateTipText( m_message.c_str(), this, ToolId );
}

bool CBalloonHostWnd::CheckMainThreadChanges( void )
{
	ASSERT( m_mainThreadId != ::GetCurrentThreadId() );		// this should run in the baloon UI thread, different than the main calling thread

	if ( NULL == m_pThreadInfo.get() )
		m_pThreadInfo.reset( new CGuiThreadInfo( m_mainThreadId ) );
	else
	{
		CGuiThreadInfo currThreadInfo( m_mainThreadId );
		CGuiThreadInfo::Change change = m_pThreadInfo->Compare( currThreadInfo );

		if ( change != CGuiThreadInfo::None )
		{
			TRACE( _T("CBalloonHostWnd: closing due to %s change!\n"), CGuiThreadInfo::GetTags_Change().FormatUi( change ).c_str() );
			return true;
		}
	}

	return false;
}

BOOL CBalloonHostWnd::PreTranslateMessage( MSG* pMsg )
{
	if ( ::IsWindow( m_toolTipCtrl.m_hWnd ) )
		m_toolTipCtrl.RelayEvent( pMsg );			// give our tooltip a shot at every message

	return CWnd::PreTranslateMessage( pMsg );
}

BOOL CBalloonHostWnd::OnNotify( WPARAM wParam, LPARAM lParam, LRESULT* pResult )
{
	NMHDR* pNmHdr = (NMHDR*)lParam;

	if ( pNmHdr->hwndFrom == m_toolTipCtrl.m_hWnd )			// send only for a normal tooltip - however, we settled on using a tracking tooltip, so there are no notifications
		switch ( pNmHdr->code )
		{
			case TTN_SHOW:
				TRACE( "CBalloonHostWnd::OnNotify - TTN_SHOW\n" );
				break;
			case TTN_POP:
				TRACE( "CBalloonHostWnd::OnNotify - Closing due to TTN_POP\n" );
				::PostQuitMessage( 0 );		// tooltip is closing
				*pResult = 0;
				return TRUE;
		}

	return CWnd::OnNotify( wParam, lParam, pResult );
}


// message handlers

BEGIN_MESSAGE_MAP( CBalloonHostWnd, CWnd )
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_TIMER()
	ON_WM_SETCURSOR()
END_MESSAGE_MAP()

int CBalloonHostWnd::OnCreate( CREATESTRUCT* pCreateStruct )
{
	if ( -1 == CWnd::OnCreate( pCreateStruct ) )
		return -1;

	VERIFY( CreateToolTip() );

	// Start the timer that monitors main thread activation changes, and self-closes (auto-pop) our tracking ballon.
	// It also ensures that the thread's OnIdle() method is called while displayed.
	m_timer.Start();
	return 0;
}

void CBalloonHostWnd::OnDestroy( void )
{
	::PostQuitMessage( 0 );			// kick off a graceful exit of the owner UI thread

	__super::OnDestroy();
}

void CBalloonHostWnd::OnTimer( UINT_PTR eventId )
{
	if ( m_timer.IsHit( eventId ) )
	{
		bool mustDestroy = false;

		if ( IsTooltipDisplayed() )
			if ( m_autoPopTimeout != 0 )
				if ( ( EllapseMs * ++m_eventCount ) >= m_autoPopTimeout )		// auto-pop timeout?
				{
					TRACE( "CBalloonHostWnd::OnTimer: self-closing due to auto-pop timeout.\n" );
					mustDestroy = true;
				}

		if ( CheckMainThreadChanges() )
			mustDestroy = true;

		if ( mustDestroy )
			DestroyWindow();
	}
	else
		CWnd::OnTimer( eventId );
}

BOOL CBalloonHostWnd::OnSetCursor( CWnd* pWnd, UINT hitTest, UINT message )
{
	pWnd, hitTest, message;
	::SetCursor( ::AfxGetApp()->LoadStandardCursor( IDC_ARROW ) );
	return TRUE;
}


// CBalloonUiThread implementation

bool CBalloonUiThread::s_exitingAll = false;

IMPLEMENT_DYNCREATE( CBalloonUiThread, CWinThread )

CBalloonUiThread::CBalloonUiThread( void )
	: CWinThread()
	, m_exiting( false )
{
	m_bAutoDelete = TRUE;
	ResetCloseAll();
}

CBalloonUiThread::~CBalloonUiThread()
{
}

BOOL CBalloonUiThread::InitInstance( void )
{
	m_pMainWnd = &m_hostWnd;

	VERIFY( m_hostWnd.Create( CWnd::GetActiveWindow() ) );
	m_hostWnd.ShowWindow( SW_SHOWNOACTIVATE );		// don't activate - we don't want the orignal window to lose its state

	TRACE( _T("STARTING: CBalloonUiThread\n") );
	return TRUE;
}

int CBalloonUiThread::ExitInstance( void )
{
	TRACE( _T("TERMINATING: CBalloonUiThread\n") );
	return CWinThread::ExitInstance();
}

BOOL CBalloonUiThread::OnIdle( LONG count )
{
	if ( m_exiting || s_exitingAll )
		m_hostWnd.DestroyWindow();		// will kick off a graceful thread exit

	return CWinThread::OnIdle( count );
}
