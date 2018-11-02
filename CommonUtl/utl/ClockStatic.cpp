
#include "stdafx.h"
#include "ClockStatic.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CClockStatic::CClockStatic( unsigned int precision /*= 0*/, Style style /*= Static*/ )
	: CRegularStatic( style )
	, m_precision( precision )
	, m_clockTimer( this, ClockTimerId, 1000 )
{
	SetPrecision( m_precision );
}

CClockStatic::~CClockStatic()
{
}

void CClockStatic::SetPrecision( unsigned int precision )
{
	ASSERT( precision <= 3 );

	m_precision = precision;
	switch ( m_precision )
	{
		case 0:	m_clockTimer.SetElapsed( 1000 ); break;
		case 1:	m_clockTimer.SetElapsed( 100 ); break;
		case 2:
		case 3:	m_clockTimer.SetElapsed( 10 ); break;
	}
}

void CClockStatic::PreSubclassWindow( void )
{
	__super::PreSubclassWindow();

	SetWindowText( str::GetEmpty() );			// clear any guide label text
	m_clockTimer.Start();
}


// message handlers

BEGIN_MESSAGE_MAP( CClockStatic, CRegularStatic )
	ON_WM_TIMER()
END_MESSAGE_MAP()

void CClockStatic::OnTimer( UINT_PTR eventId )
{
	if ( m_clockTimer.IsHit( ClockTimerId ) )
		SetWindowText( m_timer.FormatElapsedDuration( m_precision ) );
	else
		__super::OnTimer( eventId );
}
