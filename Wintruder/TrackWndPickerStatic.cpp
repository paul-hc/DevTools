
#include "pch.h"
#include "TrackWndPickerStatic.h"
#include "utl/UI/WndUtils.h"
#include "wnd/WndFinder.h"
#include "wnd/WndHighlighter.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CTrackWndPickerStatic::CTrackWndPickerStatic( void )
	: CTrackStatic()
{
}

CTrackWndPickerStatic::~CTrackWndPickerStatic()
{
}

void CTrackWndPickerStatic::BeginTracking( void )
{
	m_pickedWnd = CWndSpot::m_nullWnd;
	CTrackStatic::BeginTracking();
}

void CTrackWndPickerStatic::EndTracking( void )
{
	m_pWndHighlighter.reset();
	CTrackStatic::EndTracking();
}

void CTrackWndPickerStatic::OnTrack( void )
{
	// freeze current selected window if CONTROL key down
	if ( !ui::IsKeyPressed( VK_CONTROL ) )
	{
		CWndFinder finder;
		m_pickedWnd = finder.WindowFromPoint( GetCursorPos() );

		if ( NULL == m_pWndHighlighter.get() )
			m_pWndHighlighter.reset( new CWndHighlighter() );

		if ( m_pWndHighlighter->SetSelected( m_pickedWnd ) )		// highlighter refreshes the OLD window and highlights the NEW window
			NotifyParent( TSWN_FOUNDWINDOW );
	}
}
