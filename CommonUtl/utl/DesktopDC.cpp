
#include "stdafx.h"
#include "DesktopDC.h"
#include "Utilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CDesktopDC::CDesktopDC( bool clipTopLevelWindows /*= true*/, bool useUpdateLocking /*= false*/ )
	: CDC()
	, m_updateLocked( false )
	, m_hDesktopWnd( ::GetDesktopWindow() )
{
	if ( !Attach( useUpdateLocking ? InitUpdateLocking( clipTopLevelWindows ) : InitNormal( clipTopLevelWindows ) ) )
		TRACE( _T(" ** CDesktopDC: GetDCEx failed! **\n") );
}

CDesktopDC::~CDesktopDC()
{
	if ( m_hDC != NULL )
		Release();
}

void CDesktopDC::Release( void )
{
	ASSERT_PTR( m_hDC );
	::ReleaseDC( NULL, Detach() );

	if ( m_updateLocked )
		if ( !::LockWindowUpdate( NULL ) )
			TRACE( _T(" ** CDesktopDC: LockWindowUpdate() UNLOCK failed! **\n") );
}

HDC CDesktopDC::InitUpdateLocking( bool clipTopLevelWindows )
{
	CRgn topLevelRegion;
	if ( clipTopLevelWindows )
		MakeTopLevelRegion( topLevelRegion );

	if ( ::LockWindowUpdate( m_hDesktopWnd ) )
		m_updateLocked = true;
	else
		TRACE( _T(" ** CDesktopDC: LockWindowUpdate( hDesktop ) failed! **\n") );

	DWORD flags = DCX_WINDOW;

	if ( m_updateLocked )
		flags |= DCX_LOCKWINDOWUPDATE;
	if ( topLevelRegion.GetSafeHandle() != NULL )
		flags |= DCX_EXCLUDERGN;

	return ::GetDCEx( NULL, (HRGN)topLevelRegion.Detach(), flags );		// the system takes ownership of the region
}

HDC CDesktopDC::InitNormal( bool clipTopLevelWindows )
{
	HDC hDC = ::GetDC( NULL );						// entire desktop DC (spanning all monitors)

	if ( clipTopLevelWindows && hDC != NULL )
	{
		std::vector< HWND > topLevelWindows;		// visible ones
		ui::QueryTopLevelWindows( topLevelWindows, WS_VISIBLE );

		for ( std::vector< HWND >::const_iterator itWnd = topLevelWindows.begin(); itWnd != topLevelWindows.end(); ++itWnd )
		{
			CRect windowRect;
			::GetWindowRect( *itWnd, &windowRect );
			::ExcludeClipRect( hDC, windowRect.left, windowRect.top, windowRect.right, windowRect.bottom );
		}
	}
	return hDC;
}

void CDesktopDC::MakeTopLevelRegion( CRgn& rRegion ) const
{
	std::vector< HWND > topLevelWindows;		// visible ones
	ui::QueryTopLevelWindows( topLevelWindows, WS_VISIBLE );

	for ( std::vector< HWND >::const_iterator itWnd = topLevelWindows.begin(); itWnd != topLevelWindows.end(); ++itWnd )
	{
		CRect windowRect;
		::GetWindowRect( *itWnd, &windowRect );

		if ( NULL == (HRGN)rRegion )
			rRegion.CreateRectRgnIndirect( &windowRect );
		else
		{
			CRgn windowRgn;
			windowRgn.CreateRectRgnIndirect( &windowRect );
			rRegion.CombineRgn( &rRegion, &windowRgn, RGN_OR );
		}
	}
}
