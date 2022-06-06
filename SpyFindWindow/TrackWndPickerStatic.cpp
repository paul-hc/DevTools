
#include "stdafx.h"
#include "TrackWndPickerStatic.h"
#include "WndFinder.h"
#include "WndHighlighter.h"
#include "utl/UI/WndUtils.h"

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

const CWndSpot& CTrackWndPickerStatic::GetSelectedWnd( void ) const
{
	ASSERT( IsTracking() );
	static const CWndSpot nullWnd;
	return m_pWndHighlighter.get() != NULL ? m_pWndHighlighter->GetSelected() : nullWnd;
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
		CWndSpot wndFound = finder.WindowFromPoint( GetCursorPos() );

		if ( NULL == m_pWndHighlighter.get() )
			m_pWndHighlighter.reset( new CWndHighlighter );

		if ( m_pWndHighlighter->SetSelected( wndFound ) )		// highlighter refreshes the OLD window and highlights the NEW window
			NotifyParent( TSWN_FOUNDWINDOW );
	}
}
