
#include "stdafx.h"
#include "TrackStatic.h"
#include "ImageStore.h"
#include "Utilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CTrackStatic::CTrackStatic( void )
	: CStatic()
	, m_hTrackCursor( NULL )
	, m_trackingResult( Cancel )
{
}

CTrackStatic::~CTrackStatic()
{
	// no need to destroy m_hTrackCursor
}

void CTrackStatic::LoadTrackCursor( UINT trackCursorId )
{
	ASSERT_NULL( m_hWnd );
	m_hTrackCursor = AfxGetApp()->LoadCursor( trackCursorId );
	ASSERT_PTR( m_hTrackCursor );
}

void CTrackStatic::BeginTracking( void )
{
	ASSERT( !IsTracking() );

	CTrackData* pTrackData = new CTrackData( ui::GetCursorPos() );

	if ( ITrackToolCallback* pCallback = dynamic_cast< ITrackToolCallback* >( GetParent() ) )
		if ( pCallback->OnPreTrackMoveCursor( this ) )
		{
			pTrackData->m_cursorPos = ui::GetCursorPos();
			pTrackData->m_restoreStartPos = true;
		}

	if ( !pTrackData->m_restoreStartPos )
	{
		CPoint hotSpot = GetTrackCursorHotSpot();
		if ( hotSpot != CPoint( -1, -1 ) )
		{	// move the mouse to the cursor's hotspot in the tool
			CRect toolRect;
			GetWindowRect( &toolRect );

			pTrackData->m_cursorPos = toolRect.TopLeft() + hotSpot;
			::SetCursorPos( pTrackData->m_cursorPos.x, pTrackData->m_cursorPos.y );
		}
	}

	m_pTrackData.reset( pTrackData );		// only now enter tracking mode

	if ( m_trackIconId.IsValid() )
		if ( const CIcon* pTrackIcon = CImageStore::SharedStore()->RetrieveIcon( m_trackIconId ) )
			m_pTrackData->m_hOldIcon = SetIcon( pTrackIcon->GetHandle() );		// display the tracking image (with no syringe)

	if ( m_hTrackCursor != NULL )
		m_pTrackData->m_hOldCursor = ::SetCursor( m_hTrackCursor );				// set the tracking cursor

	m_pTrackData->m_hOldFocus = ::SetFocus( m_hWnd );							// for keyboard input

	SetCapture();
	NotifyParent( TSN_BEGINTRACKING );
}

void CTrackStatic::EndTracking( void )
{
	ASSERT( IsTracking() );

	if ( m_pTrackData->m_restoreStartPos || CursorInTool() )
		::SetCursorPos( m_pTrackData->m_startPos.x, m_pTrackData->m_startPos.y );		// restore the original start pos

	if ( m_pTrackData->m_hOldIcon != NULL )
		SetIcon( m_pTrackData->m_hOldIcon );		// restore the idle image

	if ( m_pTrackData->m_hOldCursor != NULL )
		::SetCursor( m_pTrackData->m_hOldCursor );	// restore the old cursor

	if ( IsWindow( m_pTrackData->m_hOldFocus ) )
		ui::TakeFocus( m_pTrackData->m_hOldFocus );

	m_pTrackData.reset();

	ReleaseCapture();
	NotifyParent( TSN_ENDTRACKING );
}

bool CTrackStatic::CursorInTool( void ) const
{
	CRect toolRect;
	GetWindowRect( &toolRect );
	return IsTracking() && toolRect.PtInRect( m_pTrackData->m_cursorPos ) != FALSE;
}

void CTrackStatic::OnTrack( void )
{
}

void CTrackStatic::HandleMouseMove( const CPoint& screenPos )
{
	m_pTrackData->m_delta = screenPos - m_pTrackData->m_cursorPos;
	m_pTrackData->m_cursorPos = screenPos;

	OnTrack();
	NotifyParent( TSN_TRACK );
}

void CTrackStatic::StopTracking( TrackingResult trackingResult )
{
	m_trackingResult = trackingResult;
	EndTracking();
}

void CTrackStatic::CancelTracking( void )
{
	if ( IsTracking() )
		StopTracking( Cancel );
}

CPoint CTrackStatic::GetTrackCursorHotSpot( void ) const
{
	return m_hTrackCursor != NULL ? CIconInfo( m_hTrackCursor ).GetHotSpot() : CPoint( -1, -1 );
}

void CTrackStatic::NotifyParent( int notifyCode )
{
	ui::SendCommandToParent( m_hWnd, notifyCode );
}

void CTrackStatic::PreSubclassWindow( void )
{
	CStatic::PreSubclassWindow();

	if ( NULL == GetIcon() && m_toolIconId.IsValid() )
		if ( const CIcon* pToolIcon = CImageStore::SharedStore()->RetrieveIcon( m_toolIconId ) )
		{
			ModifyStyle( SS_TYPEMASK, SS_ICON | SS_REALSIZEIMAGE | SS_NOTIFY );
			SetIcon( pToolIcon->GetHandle() );
		}
}


// message handlers

BEGIN_MESSAGE_MAP( CTrackStatic, CStatic )
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_CAPTURECHANGED()
	ON_WM_CANCELMODE()
	ON_WM_DESTROY()
END_MESSAGE_MAP()

BOOL CTrackStatic::PreTranslateMessage( MSG* pMsg )
{
	if ( WM_KEYDOWN == pMsg->message && IsTracking() )
		if ( VK_ESCAPE == pMsg->wParam )
		{
			CancelTracking();
			return TRUE;
		}

	return CStatic::PreTranslateMessage( pMsg );
}

void CTrackStatic::OnDestroy( void )
{
	CancelTracking();
	CStatic::OnDestroy();
}

void CTrackStatic::OnLButtonDown( UINT vkFlags, CPoint point )
{
	CStatic::OnLButtonDown( vkFlags, point );

	ASSERT( !IsTracking() );
	BeginTracking();
}

void CTrackStatic::OnLButtonUp( UINT vkFlags, CPoint point )
{
	CStatic::OnLButtonUp( vkFlags, point );

	if ( IsTracking() )
		StopTracking( Commit );
}

void CTrackStatic::OnMouseMove( UINT vkFlags, CPoint point )
{
	CStatic::OnMouseMove( vkFlags, point );

	if ( IsTracking() )
	{
		CPoint screenPos = point;
		ClientToScreen( &screenPos );
		HandleMouseMove( screenPos );
	}
}

void CTrackStatic::OnCaptureChanged( CWnd *pWnd )
{
	if ( IsTracking() && ::GetCapture() != m_hWnd )
		CancelTracking();

	CStatic::OnCaptureChanged( pWnd );
}

void CTrackStatic::OnCancelMode( void )
{
	CStatic::OnCancelMode();
	CancelTracking();
}
