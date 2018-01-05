
#include "stdafx.h"
#include "WndSearchPattern.h"
#include "WindowClass.h"
#include "utl/Utilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace reg
{
	static const TCHAR section_search[] = _T("SearchPattern");
	static const TCHAR entry_fromBeginning[] = _T("FromBeginning");
	static const TCHAR entry_forward[] = _T("Forward");
	static const TCHAR entry_matchCase[] = _T("MatchCase");
	static const TCHAR entry_matchWhole[] = _T("MatchWhole");
	static const TCHAR entry_refreshNow[] = _T("RefreshNow");
	static const TCHAR entry_id[] = _T("Id");
	static const TCHAR entry_wndClass[] = _T("WndClass");
	static const TCHAR entry_caption[] = _T("Caption");
}


CWndSearchPattern::CWndSearchPattern( void )
	: m_fromBeginning( false )
	, m_forward( true )
	, m_matchCase( false )
	, m_matchWhole( false )
	, m_refreshNow( false )
	, m_id( INT_MAX )
	, m_handle( NULL )
{
}

void CWndSearchPattern::Load( void )
{
	CWinApp* pApp = AfxGetApp();

	m_fromBeginning = pApp->GetProfileInt( reg::section_search, reg::entry_fromBeginning, m_fromBeginning ) != FALSE;
	m_forward = pApp->GetProfileInt( reg::section_search, reg::entry_forward, m_forward ) != FALSE;
	m_matchCase = pApp->GetProfileInt( reg::section_search, reg::entry_matchCase, m_matchCase ) != FALSE;
	m_matchWhole = pApp->GetProfileInt( reg::section_search, reg::entry_matchWhole, m_matchWhole ) != FALSE;
	m_refreshNow = pApp->GetProfileInt( reg::section_search, reg::entry_refreshNow, m_refreshNow ) != FALSE;
	m_id = pApp->GetProfileInt( reg::section_search, reg::entry_id, m_id );
	m_wndClass = (LPCTSTR)pApp->GetProfileString( reg::section_search, reg::entry_wndClass, m_wndClass.c_str() );
	m_caption = (LPCTSTR)pApp->GetProfileString( reg::section_search, reg::entry_caption, m_caption.c_str() );
}

void CWndSearchPattern::Save( void ) const
{
	CWinApp* pApp = AfxGetApp();

	pApp->WriteProfileInt( reg::section_search, reg::entry_fromBeginning, m_fromBeginning );
	pApp->WriteProfileInt( reg::section_search, reg::entry_forward, m_forward );
	pApp->WriteProfileInt( reg::section_search, reg::entry_matchCase, m_matchCase );
	pApp->WriteProfileInt( reg::section_search, reg::entry_matchWhole, m_matchWhole );
	pApp->WriteProfileInt( reg::section_search, reg::entry_refreshNow, m_refreshNow );
	pApp->WriteProfileInt( reg::section_search, reg::entry_id, m_id );
	pApp->WriteProfileString( reg::section_search, reg::entry_wndClass, m_wndClass.c_str() );
	pApp->WriteProfileString( reg::section_search, reg::entry_caption, m_caption.c_str() );
}

bool CWndSearchPattern::Matches( HWND hWnd ) const
{
	ASSERT( ui::IsValidWindow( hWnd ) );

	if ( m_id != INT_MAX )
		if ( !HasFlag( ui::GetStyle( hWnd ), WS_CHILD ) || m_id != ::GetDlgCtrlID( hWnd ) )
			return false;

	if ( !m_wndClass.empty() )
	{
		std::tstring wndClass = wc::GetClassName( hWnd );

		// keep m_wndClass on the right-hand side so that it makes a correct sub-string match
		if ( !IsMatch( wndClass, m_wndClass ) && !IsMatch( wc::GetDisplayClassName( hWnd ), m_wndClass ) )
			return false;
	}

	if ( !m_caption.empty() )
		if ( !IsMatch( ui::GetWindowText( hWnd ), m_caption ) )
			return false;

	return true;
}

std::tstring CWndSearchPattern::FormatNotFound( void ) const
{
	std::tstring message; message.reserve( 128 );
	static const TCHAR sep[] = _T("\n");

	if ( m_handle != NULL )
		stream::Tag( message, str::Format( _T("Handle: %08X"), m_handle ), sep );
	if ( !m_wndClass.empty() )
		stream::Tag( message, str::Format( _T("Window Class: %s"), wc::FormatClassName( m_wndClass.c_str() ).c_str() ), sep );
	if ( !m_caption.empty() )
		stream::Tag( message, str::Format( _T("Caption: \"%s\""), m_caption.c_str() ), sep );
	if ( m_id != INT_MAX )
		stream::Tag( message, str::Format( _T("ID: %d (0x%08X)"), m_id, m_id ), sep );
	return message;
}
