
#include "pch.h"
#include "WindowTimer.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CWindowTimer implementation

CWindowTimer::CWindowTimer( CWnd* pWnd, UINT_PTR timerId, UINT elapseMs )
	: m_pWnd( pWnd )
	, m_timerId( timerId )
	, m_elapseMs( elapseMs )
	, m_eventId( 0 )
{
	ASSERT_PTR( m_pWnd );
	ASSERT( m_timerId != 0 && m_elapseMs != 0 );
}

CWindowTimer::~CWindowTimer()
{
	Stop();
}

void CWindowTimer::Start( void )
{
	ASSERT( IsWindow( m_pWnd->GetSafeHwnd() ) );

	Stop();
	m_eventId = m_pWnd->SetTimer( m_timerId, m_elapseMs, nullptr );

	ENSURE( IsStarted() );
}

void CWindowTimer::Start( UINT elapseMs )
{
	SetElapsed( elapseMs );

	if ( !IsStarted() )
		Start();
}

void CWindowTimer::Stop( void )
{
	if ( IsStarted() )
		VERIFY( m_pWnd->KillTimer( m_eventId ) );

	m_eventId = 0;
}

void CWindowTimer::SetElapsed( UINT elapseMs )
{
	ASSERT( elapseMs != 0 );
	m_elapseMs = elapseMs;

	if ( IsStarted() )
		Start();			// restart
}


// CTimerSequenceHook implementation

const UINT CTimerSequenceHook::WM_ENDTIMERSEQ = RegisterWindowMessageA( "utl:WM_ENDTIMERSEQ" );		// wParam: m_eventId

CTimerSequenceHook::CTimerSequenceHook( ISequenceTimerCallback* pCallback )
	: CWindowHook( false )
	, m_pCallback( pCallback )
	, m_eventId( 0 )
	, m_seqCount( 0 )
{
	ASSERT_PTR( m_pCallback );
}

CTimerSequenceHook::CTimerSequenceHook( HWND hWnd, ISequenceTimerCallback* pCallback, int eventId, size_t seqCount, UINT elapseMs )
	: CWindowHook( true )
	, m_pCallback( pCallback )
	, m_eventId( ::SetTimer( hWnd, eventId, elapseMs, nullptr ) )
	, m_seqCount( seqCount )
{
	ASSERT( m_pCallback != nullptr && m_seqCount != 0 );
	HookWindow( hWnd );
}

CTimerSequenceHook::~CTimerSequenceHook()
{
	if ( IsHooked() )		// restartable timer still on?
		Stop();
}

void CTimerSequenceHook::Start( HWND hWnd, int eventId, UINT elapseMs, size_t seqCount /*= 1*/ )
{
	REQUIRE( !m_autoDelete );			// created by contructor 1
	REQUIRE( 0 == m_eventId );
	REQUIRE( ::IsWindow( hWnd ) );

	if ( IsHooked() )
		Stop();

	HookWindow( hWnd );
	m_eventId = ::SetTimer( hWnd, eventId, elapseMs, nullptr );
	m_seqCount = seqCount;
}

void CTimerSequenceHook::Stop( void )
{
	ASSERT( IsHooked() );

	if ( m_eventId != 0 )
	{
		VERIFY( ::KillTimer( m_hWnd, m_eventId ) );
		::SendMessage( m_hWnd, WM_ENDTIMERSEQ, m_eventId, 0 );
		m_eventId = 0;
	}

	UnhookWindow();			// unhook, delete this
}

LRESULT CTimerSequenceHook::WindowProc( UINT message, WPARAM wParam, LPARAM lParam ) override
{
	if ( WM_TIMER == message )
		if ( m_eventId == wParam )
		{
			ASSERT( m_seqCount != 0 );
			m_pCallback->OnSequenceStep();
			if ( 0 == --m_seqCount )			// timer sequence is completed
				Stop();
			return 0L;
		}

	return CWindowHook::WindowProc( message, wParam, lParam );
}
