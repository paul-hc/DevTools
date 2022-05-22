
#include "stdafx.h"
#include "WndEnumerator.h"
#include "WindowClass.h"
#include "utl/UI/Utilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace wnd
{
	void QueryAncestorBranchPath( std::vector< HWND >& rBranchPath, HWND hWnd, HWND hWndAncestor /*= ::GetDesktopWindow()*/ )	// bottom-up ancestor path order from [hWnd, ..., hWndAncestor]
	{
		for ( ; hWnd != NULL; hWnd = ::GetAncestor( hWnd, GA_PARENT ) )		// real parent, NULL if desktop - GetParent( hWnd ) may return the owner
		{
			rBranchPath.push_back( hWnd );

			if ( hWnd == hWndAncestor )
				break;
		}
	}
}


// CWndEnumBase implementation

CWndEnumBase::CWndEnumBase( void )
	: m_appProcessId( ui::GetWindowProcessId( AfxGetMainWnd()->GetSafeHwnd() ) )
{
}

bool CWndEnumBase::IsValidMatch( HWND hWnd ) const
{
	return ::IsWindow( hWnd  ) && ui::GetWindowProcessId( hWnd ) != m_appProcessId;		// ignore windows belonging to this process
}

void CWndEnumBase::Build( HWND hRootWnd )
{
	ASSERT( ::IsWindow( hRootWnd ) );

	AddWndItem( hRootWnd );
	BuildChildren( hRootWnd );
}

void CWndEnumBase::BuildChildren( HWND hWnd )
{
	if ( hWnd == ::GetDesktopWindow() )
		::EnumWindows( (WNDENUMPROC)&EnumWindowProc, reinterpret_cast<LPARAM>( this ) );
	else
		::EnumChildWindows( hWnd, (WNDENUMPROC)&EnumChildWindowProc, reinterpret_cast<LPARAM>( this ) );
}

BOOL CALLBACK CWndEnumBase::EnumWindowProc( HWND hTopLevel, CWndEnumBase* pEnumerator )
{
	if ( pEnumerator->IsValidMatch( hTopLevel ) )
		if ( wc::GetClassName( hTopLevel ) != _T("SysFader") )		// avoid this annoying temporary system window
		{
			pEnumerator->AddWndItem( hTopLevel );
			::EnumChildWindows( hTopLevel, (WNDENUMPROC)&EnumChildWindowProc, reinterpret_cast<LPARAM>( pEnumerator ) );
		}

	return TRUE;			// continue enumeration
}

BOOL CALLBACK CWndEnumBase::EnumChildWindowProc( HWND hWnd, CWndEnumBase* pEnumerator )
{
	if ( pEnumerator->IsValidMatch( hWnd ) )
		pEnumerator->AddWndItem( hWnd );
	return TRUE;			// continue enumeration
}


// CWndEnumerator implementation

void CWndEnumerator::AddWndItem( HWND hWnd )
{
	m_windows.push_back( hWnd );
}
