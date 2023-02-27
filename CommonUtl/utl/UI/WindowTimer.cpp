
#include "stdafx.h"
#include "WindowTimer.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CWindowTimer implementation

CWindowTimer::CWindowTimer( CWnd* pWnd, UINT_PTR timerId, UINT elapsedMs )
	: m_pWnd( pWnd )
	, m_timerId( timerId )
	, m_elapsedMs( elapsedMs )
	, m_eventId( 0 )
{
	ASSERT_PTR( m_pWnd );
	ASSERT( m_timerId != 0 && m_elapsedMs != 0 );
}

CWindowTimer::~CWindowTimer()
{
	Stop();
}

void CWindowTimer::Start( void )
{
	ASSERT( IsWindow( m_pWnd->GetSafeHwnd() ) );

	Stop();
	m_eventId = m_pWnd->SetTimer( m_timerId, m_elapsedMs, nullptr );

	ENSURE( IsStarted() );
}

void CWindowTimer::Start( UINT elapsedMs )
{
	SetElapsed( elapsedMs );

	if ( !IsStarted() )
		Start();
}

void CWindowTimer::Stop( void )
{
	if ( IsStarted() )
		VERIFY( m_pWnd->KillTimer( m_eventId ) );

	m_eventId = 0;
}

void CWindowTimer::SetElapsed( UINT elapsedMs )
{
	ASSERT( elapsedMs != 0 );
	m_elapsedMs = elapsedMs;

	if ( IsStarted() )
		Start();			// restart
}


// CTimerSequenceHook implementation

const UINT CTimerSequenceHook::WM_ENDTIMERSEQ = RegisterWindowMessageA( "utl:WM_ENDTIMERSEQ" );		// wParam: m_eventId

CTimerSequenceHook::CTimerSequenceHook( HWND hWnd, ISequenceTimerCallback* pCallback, int eventId, unsigned int seqCount, int elapse )
	: CWindowHook( true )
	, m_pCallback( pCallback )
	, m_eventId( ::SetTimer( hWnd, eventId, elapse, nullptr ) )
	, m_seqCount( seqCount )
{
	ASSERT( m_pCallback != nullptr && m_seqCount != 0 );
	HookWindow( hWnd );
}

CTimerSequenceHook::~CTimerSequenceHook()
{
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

LRESULT CTimerSequenceHook::WindowProc( UINT message, WPARAM wParam, LPARAM lParam )
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
