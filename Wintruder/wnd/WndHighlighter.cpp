
#include "stdafx.h"
#include "WndHighlighter.h"
#include "AppService.h"
#include "utl/ContainerUtilities.h"
#include "utl/UI/DesktopDC.h"
#include "utl/UI/Utilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


bool CWndHighlighter::m_cacheDesktopDC = true;
bool CWndHighlighter::m_redrawAtEnd = true;

CWndHighlighter::~CWndHighlighter()
{
	if ( IsDrawn() && m_wndSpot.IsValid() )
		SetSelected( CWndSpot() );		// revert last draw

	m_pScreenDC.reset();
	if ( IsDirty() && m_finalRedraw )
		RedrawDirtyWindows();
}

void CWndHighlighter::RedrawDirtyWindows( void )
{
	for ( std::vector< HWND >::const_iterator itWnd = m_dirtyWnds.begin(); itWnd != m_dirtyWnds.end(); ++itWnd )
		ui::RedrawWnd( *itWnd );
}

void CWndHighlighter::FlashWnd( HWND hOwnerWnd, const CWndSpot& wndSpot, unsigned int count /*= 4*/, int elapse /*= 150*/ )
{
	SetSelected( wndSpot );
	new CTimerSequenceHook( hOwnerWnd, this, FlashTimerEvent, count * 2 - 1, elapse );		// decrement since first step is drawn right away
}

void CWndHighlighter::OnSequenceStep( void )
{
	DrawWindowFrame();
}

bool CWndHighlighter::SetSelected( const CWndSpot& wndSpot )
{
	if ( m_wndSpot.Equals( wndSpot ) )
		return false;		// not changed

	if ( m_wndSpot.IsValid() )
		DrawWindowFrame();

	m_wndSpot = wndSpot;

	if ( m_wndSpot.IsValid() )
	{
		utl::AddUnique( m_dirtyWnds, m_wndSpot.m_hWnd );
		DrawWindowFrame();
	}
	return true;
}

void CWndHighlighter::DrawWindowFrame( void )
{
	if ( !m_wndSpot.IsValid() )
		return;

	// create screen DC on first draw
	if ( NULL == m_pScreenDC.get() || !m_cacheDesktopDC )
		m_pScreenDC.reset( new CDesktopDC );

	DrawWindowFrame( m_pScreenDC.get(), m_wndSpot );
	if ( !m_cacheDesktopDC )
		m_pScreenDC.reset();
	++m_drawCount;

	//TRACE( _T("CWndHighlighter::DrawWindowFrame() - %s\n"), ( m_drawCount % 2 ) ? _T("Frame Drawn") : _T("Frame Erased") );
}

void CWndHighlighter::DrawWindowFrame( CDC* pDC, const CWndSpot& wndSpot )
{
	ASSERT( pDC != NULL && wndSpot.IsValid() );

	CRect windowRect = wndSpot.GetWindowRect();
	CRect clientRect;
	::GetClientRect( wndSpot.m_hWnd, &clientRect );
	ui::ClientToScreen( wndSpot.m_hWnd, clientRect );	// client rect in screen coordinates

	CRect monitorRect = wndSpot.FindMonitorRect();		// monitor corresponding to test point
	windowRect &= monitorRect;							// constrain to monitor rect
	clientRect &= monitorRect;

	int frameSize = std::min< int >( clientRect.Width() / 2, app::GetOptions()->m_frameSize );
	frameSize = std::max< int >( 1, frameSize );

	switch ( app::GetOptions()->m_frameStyle )
	{
		default: ASSERT( false );
		case opt::EntireWindow:
			pDC->InvertRect( &windowRect );
			break;
		case opt::NonClient:
			if ( clientRect.Size() != windowRect.Size() )
			{
				CRgn ncRegion;
				ncRegion.CreateRectRgnIndirect( &windowRect );
				ui::EnsureMinEdge( clientRect, windowRect, 1 );		// ensure a minimum non-client edge

				if ( ui::CombineWithRegion( &ncRegion, clientRect, RGN_DIFF ) != NULLREGION )
				{
					pDC->InvertRgn( &ncRegion );
					break;
				}
			}
			// fall-through
		case opt::Frame:
		{
			CPen pen( PS_SOLID | PS_INSIDEFRAME, frameSize, ui::GetInverseColor( FillColor ) );
			CPen* pOldPen = pDC->SelectObject( &pen );
			CGdiObject* pOldBrush = pDC->SelectStockObject( NULL_BRUSH );
			int oldRop2 = pDC->SetROP2( R2_XORPEN );

			pDC->Rectangle( &windowRect );

			pDC->SelectObject( pOldPen );
			pDC->SelectObject( pOldBrush );
			pDC->SetROP2( oldRop2 );
			break;
		}
	}
}
