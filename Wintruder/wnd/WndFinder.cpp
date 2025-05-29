
#include "pch.h"
#include "WndFinder.h"
#include "WndEnumerator.h"
#include "WndSearchPattern.h"
#include "WndUtils.h"
#include "AppService.h"
#include "utl/Algorithms.h"
#include "utl/UI/ProcessUtils.h"
#include "utl/UI/WndUtils.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace pred
{
	struct Matches
	{
		Matches( const CWndSearchPattern* pPattern ) : m_pPattern( pPattern ) { ASSERT_PTR( m_pPattern ); }

		bool operator()( HWND hWnd ) const
		{
			return m_pPattern->Matches( hWnd );
		}
	private:
		const CWndSearchPattern* m_pPattern;
	};
}


CWndFinder::CWndFinder( void )
	: m_appProcessId( 0 )
	, m_thisThreadId( ::GetWindowThreadProcessId( AfxGetMainWnd()->GetSafeHwnd(), &m_appProcessId ) )
{
	ASSERT( IsWindow( AfxGetMainWnd()->GetSafeHwnd() ) );
	ASSERT( m_appProcessId != 0 );
}

bool CWndFinder::IsValidMatch( HWND hWnd ) const
{	// ignore windows belonging to this app's process
	if ( !ui::IsValidWindow( hWnd  ) )
		return false;

	DWORD processId = 0;
	::GetWindowThreadProcessId( hWnd, &processId );
	if ( m_appProcessId == processId )
		return false;

	return true;
}

bool CWndFinder::IsValidMatchIgnore( HWND hWnd ) const
{
	if ( !IsValidMatch( hWnd ) )
		return false;
	if ( app::GetOptions()->m_ignoreDisabled && !::IsWindowEnabled( hWnd ) )
		return false;
	if ( app::GetOptions()->m_ignoreHidden && !::IsWindowVisible( hWnd ) )
		return false;
	return true;
}

CWndSpot CWndFinder::WindowFromPoint( const CPoint& screenPos ) const
{
	CWndSpot wndSpot( ::WindowFromPoint( screenPos ), screenPos );
	if ( !wndSpot.IsValid() || !IsValidMatchIgnore( wndSpot.m_hWnd ) )
		return CWndSpot( nullptr, screenPos );

	HWND hParentWnd = ::GetParent( wndSpot.m_hWnd );
	CRect hitRect = wndSpot.GetWindowRect();

	if ( wndSpot.IsChildWindow() )
	{	// search again for child windows (more accurate)
		CPoint clientPoint = screenPos;
		::ScreenToClient( hParentWnd, &clientPoint );
		int cwpFlags = CWP_ALL;

		if ( app::GetOptions()->m_ignoreHidden )
			SetFlag( cwpFlags, CWP_SKIPINVISIBLE );
		if ( app::GetOptions()->m_ignoreDisabled )
			SetFlag( cwpFlags, CWP_SKIPDISABLED );

		if ( HWND hChildWnd = ::ChildWindowFromPointEx( hParentWnd, clientPoint, cwpFlags ) )		// found and not ignored?
		{
			wndSpot.m_hWnd = hChildWnd;
			hitRect = wndSpot.GetWindowRect();
			wndSpot.m_hWnd = FindBestFitSibling( wndSpot.m_hWnd, hitRect, screenPos );
		}
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

	for ( HWND hSibling = ::GetWindow( hWnd, GW_HWNDFIRST ); hSibling != nullptr; hSibling = ::GetWindow( hSibling, GW_HWNDNEXT ) )
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
		if ( hChild != nullptr )
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

HWND CWndFinder::FindWindow( const CWndSearchPattern& pattern, HWND hStartWnd /*= nullptr*/ )
{
	if ( pattern.m_handle != nullptr )
		return ::IsWindow( pattern.m_handle ) ? pattern.m_handle : nullptr;

	CWndEnumerator enumerator;
	enumerator.Build( ::GetDesktopWindow() );

	const std::vector< HWND >& windows = enumerator.GetWindows();

	if ( pattern.m_fromBeginning )
		hStartWnd = nullptr;

	if ( pattern.m_forward )
		return utl::CircularFind( windows.begin(), windows.end(), hStartWnd, pred::Matches( &pattern ) );
	else
		return utl::CircularFind( windows.rbegin(), windows.rend(), hStartWnd, pred::Matches( &pattern ) );
}

CWndSpot CWndFinder::FindUpdateTarget( void ) const
{
	CWndSpot foundWnd;
	switch ( app::GetOptions()->m_updateTarget )
	{
		default:
			ASSERT( false );
		case opt::CurrentWnd:
			if ( CWndSpot* pCurrentWnd = GetValidTargetWnd( app::Beep ) )
				foundWnd = *pCurrentWnd;
			break;
		case opt::AtMouseWnd:
			return WindowFromPoint( ui::GetCursorPos() );
		case opt::ForegroundWnd:
			foundWnd.SetWnd( ::GetForegroundWindow() );
			break;
		case opt::ActiveWnd:
			foundWnd = FindActiveWnd();
			break;
		case opt::FocusedWnd:
			foundWnd = FindFocusedWnd();
			break;
		case opt::CapturedWnd:
			foundWnd = FindCapturedWnd();
			break;
		case opt::TopmostWnd:
		case opt::TopmostPopupWnd:
		case opt::TopmostVisibleWnd:
			return FindTopmostWnd( app::GetOptions()->m_updateTarget );
	}

	if ( !IsValidMatch( foundWnd.m_hWnd ) )
		foundWnd.SetWnd( nullptr );

	return foundWnd;
}

HWND CWndFinder::FindActiveWnd( void ) const
{
	if ( HWND hForegroundWnd = ::GetForegroundWindow() )
	{
		CScopedAttachThreadInput scopedThreadAccess( hForegroundWnd );
		return ::GetActiveWindow();
	}
	return nullptr;
}

HWND CWndFinder::FindFocusedWnd( void ) const
{
	if ( HWND hForegroundWnd = ::GetForegroundWindow() )
	{
		CScopedAttachThreadInput scopedThreadAccess( hForegroundWnd );
		return ::GetFocus();
	}
	return nullptr;
}

HWND CWndFinder::FindCapturedWnd( void ) const
{
	if ( HWND hForegroundWnd = ::GetForegroundWindow() )
	{
		CScopedAttachThreadInput scopedThreadAccess( hForegroundWnd );
		return ::GetCapture();
	}
	return nullptr;
}

HWND CWndFinder::FindTopmostWnd( opt::UpdateTarget topmostTarget ) const
{
	HWND hShellTrayWnd = ::FindWindow( _T("Shell_TrayWnd"), nullptr );
	HWND hFoundWnd = nullptr;

	for ( HWND hWnd = ::GetWindow( ::GetDesktopWindow(), GW_CHILD ); hWnd != nullptr; hWnd = ::GetWindow( hWnd, GW_HWNDNEXT ) )
		if ( IsValidMatch( hWnd ) )
			switch ( topmostTarget )
			{
				case opt::TopmostWnd:
					return hWnd;
				case opt::TopmostPopupWnd:
				case opt::TopmostVisibleWnd:
				{
					DWORD style = ui::GetStyle( hWnd );
					bool hit = false;

					if ( topmostTarget == opt::TopmostPopupWnd )
						hit = !HasFlag( style, WS_CHILD );
					else
						hit = HasFlag( style, WS_VISIBLE ) && !HasFlag( style, WS_MINIMIZE );

					if ( hit )
						if ( hShellTrayWnd == hWnd )
							hFoundWnd = hWnd;		// second chance to find something more interesting
						else
							return hWnd;
					break;
				}
				default:
					ASSERT( false );
			}

	return hFoundWnd;
}
