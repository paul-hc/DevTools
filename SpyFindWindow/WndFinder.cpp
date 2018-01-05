
#include "stdafx.h"
#include "WndFinder.h"
#include "Application.h"
#include "utl/Utilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CWndFinder::CWndFinder( void )
	: m_appProcessId( 0 )
{
	ASSERT( IsWindow( AfxGetMainWnd()->GetSafeHwnd() ) );
	::GetWindowThreadProcessId( AfxGetMainWnd()->GetSafeHwnd(), &m_appProcessId );
	ASSERT( m_appProcessId != 0 );
}

bool CWndFinder::IsValidMatch( HWND hWnd ) const
{	// ignore windows belonging to this app's process
	if ( hWnd != NULL && IsWindow( hWnd ) )
	{
		DWORD processId = 0;
		::GetWindowThreadProcessId( hWnd, &processId );
		if ( processId != m_appProcessId )
			return true;
	}
	return false;
}

CWndSpot CWndFinder::WindowFromPoint( const CPoint& screenPos ) const
{
	CWndSpot wndSpot( ::WindowFromPoint( screenPos ), screenPos );
	if ( !wndSpot.IsValid() || !IsValidMatch( wndSpot.m_hWnd ) )
		return CWndSpot( NULL, screenPos );

	HWND hParentWnd = ::GetParent( wndSpot.m_hWnd );
	CRect hitRect = wndSpot.GetWindowRect();

	if ( wndSpot.IsChildWindow() )
	{	// search again for child windows (more accurate)
		CPoint clientPoint = screenPos;
		::ScreenToClient( hParentWnd, &clientPoint );
		int cwpFlags = CWP_ALL;

		if ( app::GetOptions()->m_ignoreHidden )
			cwpFlags |= CWP_SKIPINVISIBLE;
		if ( app::GetOptions()->m_ignoreDisabled )
			cwpFlags |= CWP_SKIPDISABLED;

		wndSpot.m_hWnd = ::ChildWindowFromPointEx( hParentWnd, clientPoint, cwpFlags );
		ASSERT_PTR( wndSpot.m_hWnd );
		hitRect = wndSpot.GetWindowRect();
		wndSpot.m_hWnd = FindBestFitSibling( wndSpot.m_hWnd, hitRect, screenPos );
	}

	ASSERT( wndSpot.IsValid() );

	if ( HWND hChild = FindChildWindow( wndSpot.m_hWnd, hitRect, screenPos ) )
		wndSpot.m_hWnd = hChild;
	return wndSpot;
}

HWND CWndFinder::FindBestFitSibling( HWND hWnd, const CRect& hitRect, const CPoint& screenPos ) const
{
	ASSERT_PTR( hWnd );
	bool foundAny = false;

	for ( HWND hSibling = ::GetWindow( hWnd, GW_HWNDFIRST ); hSibling != NULL; hSibling = ::GetWindow( hSibling, GW_HWNDNEXT ) )
		if ( ContainsPoint( hSibling, screenPos ) )
		{
			CRect siblingRect;
			::GetWindowRect( hSibling, &siblingRect );

			if ( ( siblingRect & hitRect ) == siblingRect )
			{
				CRect windowRect;
				::GetWindowRect( hWnd, &windowRect );

				if ( !foundAny || GetRectArea( siblingRect ) < GetRectArea( windowRect ) )
				{
					hWnd = hSibling;
					foundAny = true;
				}
			}
		}

	return hWnd;
}

HWND CWndFinder::FindChildWindow( HWND hWndParent, const CRect& hitRect, const CPoint& screenPos ) const
{
	if ( HWND hChild = ::GetWindow( hWndParent, GW_CHILD ) )
	{
		hChild = FindBestFitSibling( hChild, hitRect, screenPos );
		if ( hChild != NULL )
			if ( ContainsPoint( hChild, screenPos ) )
				return hChild;
	}

	return hWndParent;
}

bool CWndFinder::ContainsPoint( HWND hWnd, const CPoint& screenPos ) const
{
	ASSERT_PTR( hWnd );

	if ( app::GetOptions()->m_ignoreDisabled && !::IsWindowEnabled( hWnd ) )
		return false;

	if ( app::GetOptions()->m_ignoreHidden && !::IsWindowVisible( hWnd ) )
		return false;
	else if ( !app::GetOptions()->m_ignoreHidden && HasFlag( ui::GetStyle( hWnd ), WS_CHILD ) )
		if ( !::IsWindowVisible( ::GetParent( hWnd ) ) )		// allow hidden child windows only if parent is visible 
			return false;

	CRect windowRect;
	::GetWindowRect( hWnd, &windowRect );
	return windowRect.PtInRect( screenPos ) != FALSE;
}
