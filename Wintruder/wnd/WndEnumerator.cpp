
#include "stdafx.h"
#include "WndEnumerator.h"
#include "WindowClass.h"
#include "utl/UI/Utilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif



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

	if ( ::GetDesktopWindow() == hRootWnd )
		::EnumWindows( (WNDENUMPROC)&EnumWindowProc, reinterpret_cast<LPARAM>( this ) );
	else
		::EnumChildWindows( hRootWnd, (WNDENUMPROC)&EnumChildWindowProc, reinterpret_cast<LPARAM>( this ) );
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
