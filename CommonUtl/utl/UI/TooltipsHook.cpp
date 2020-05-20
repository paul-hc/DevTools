
#include "stdafx.h"
#include "TooltipsHook.h"
#include "LayoutMetrics.h"
#include "CmdInfoStore.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CTooltipsHook::CTooltipsHook( HWND hWndToHook /*= NULL*/ )
	: CWindowHook()
	, m_pCustomCmdInfo( NULL )
{
	if ( hWndToHook != NULL )
		HookWindow( hWndToHook );
}

void CTooltipsHook::HookControl( CWnd* pCtrlToHook )
{
	HookWindow( pCtrlToHook->GetSafeHwnd() );
	m_pCustomCmdInfo = dynamic_cast< ui::ICustomCmdInfo* >( pCtrlToHook->GetParent() );
}

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

bool CTooltipsHook::OnTtnNeedText( NMHDR* pNmHdr )
{
	if ( ui::CCmdInfoStore::Instance().HandleTooltipNeedText( pNmHdr, NULL, m_pCustomCmdInfo ) )
		return true;		// message handled

	return false;
}
