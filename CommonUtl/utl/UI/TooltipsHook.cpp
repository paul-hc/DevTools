
#include "stdafx.h"
#include "TooltipsHook.h"
#include <afxpriv.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


LRESULT CTooltipsHook::WindowProc( UINT message, WPARAM wParam, LPARAM lParam )
{
	if ( WM_NOTIFY == message )
	{
		NMHDR* pNmHdr = (NMHDR*)lParam;

		if ( pNmHdr->hwndFrom != NULL && ( pNmHdr->code == TTN_NEEDTEXTW || pNmHdr->code == TTN_NEEDTEXTA ) )
			if ( OnTtnNeedText( pNmHdr ) )
				return TRUE;
	}

	return CWindowHook::WindowProc( message, wParam, lParam );
}

struct FriendlyCmdTarget : CCmdTarget { using CCmdTarget::GetRoutingFrame_; };

bool CTooltipsHook::OnTtnNeedText( NMHDR* pNmHdr )
{
	ASSERT( TTN_NEEDTEXTA == pNmHdr->code || TTN_NEEDTEXTW == pNmHdr->code );

	// allow top level routing frame to handle the message
	if ( FriendlyCmdTarget::GetRoutingFrame_() != NULL )
		return FALSE;

	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	// need to handle both ANSI and UNICODE versions of the message
	TOOLTIPTEXTA* pTTTA = (TOOLTIPTEXTA*)pNmHdr;
	TOOLTIPTEXTW* pTTTW = (TOOLTIPTEXTW*)pNmHdr;
	TCHAR szFullText[256];
	CString strTipText;
	UINT_PTR cmdId = pNmHdr->idFrom;
	if ( TTN_NEEDTEXTA == pNmHdr->code && ( pTTTA->uFlags & TTF_IDISHWND ) ||
		 TTN_NEEDTEXTW == pNmHdr->code && ( pTTTW->uFlags & TTF_IDISHWND ) )
		cmdId = ( (UINT)(WORD)::GetDlgCtrlID( (HWND)cmdId ) );			// idFrom is actually the HWND of the tool

	if ( cmdId != 0 )		// will be zero on a separator
	{
		AfxLoadString( static_cast< UINT >( cmdId ), szFullText );		// this is the command id, not the button index
		AfxExtractSubString( strTipText, szFullText, 1, '\n' );
	}
#ifndef _UNICODE
	if ( TTN_NEEDTEXTA == pNmHdr->code )
		lstrcpyn( pTTTA->szText, strTipText, COUNT_OF( pTTTA->szText ) );
	else
		_mbstowcsz( pTTTW->szText, strTipText, COUNT_OF( pTTTW->szText ) );
#else
	if ( TTN_NEEDTEXTA == pNmHdr->code )
		_wcstombsz( pTTTA->szText, strTipText, COUNT_OF( pTTTA->szText ) );
	else
		lstrcpyn( pTTTW->szText, strTipText, COUNT_OF( pTTTW->szText ) );
#endif

	::SetWindowPos( pNmHdr->hwndFrom, HWND_TOP, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOMOVE );		// bring the tooltip window above other popup windows
	return TRUE;    // message was handled
}
