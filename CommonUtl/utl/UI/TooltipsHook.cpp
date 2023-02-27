
#include "stdafx.h"
#include "TooltipsHook.h"
#include "CmdInfoStore.h"
#include "Dialog_fwd.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CTooltipsHook::CTooltipsHook( HWND hWndToHook /*= nullptr*/ )
	: CWindowHook()
	, m_pCustomCmdInfo( nullptr )
{
	if ( hWndToHook != nullptr )
		HookWindow( hWndToHook );
}

void CTooltipsHook::HookControl( CWnd* pCtrlToHook )
{
	HookWindow( pCtrlToHook->GetSafeHwnd() );
	m_pCustomCmdInfo = dynamic_cast<ui::ICustomCmdInfo*>( pCtrlToHook->GetParent() );
}

LRESULT CTooltipsHook::WindowProc( UINT message, WPARAM wParam, LPARAM lParam )
{
	if ( WM_NOTIFY == message )
	{
		NMHDR* pNmHdr = (NMHDR*)lParam;

		if ( pNmHdr->hwndFrom != nullptr && ( pNmHdr->code == TTN_NEEDTEXTW || pNmHdr->code == TTN_NEEDTEXTA ) )
			if ( OnTtnNeedText( pNmHdr ) )
				return TRUE;
	}

	return CWindowHook::WindowProc( message, wParam, lParam );
}

bool CTooltipsHook::OnTtnNeedText( NMHDR* pNmHdr )
{
	if ( ui::CCmdInfoStore::Instance().HandleTooltipNeedText( pNmHdr, nullptr, m_pCustomCmdInfo ) )
		return true;		// message handled

	return false;
}
