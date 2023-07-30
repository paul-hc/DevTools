
#include "pch.h"
#include "TooltipsHook.h"
#include "CmdInfoStore.h"
#include "Dialog_fwd.h"
#include "Control_fwd.h"
#include "ui_fwd.h"

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

LRESULT CTooltipsHook::WindowProc( UINT message, WPARAM wParam, LPARAM lParam ) override
{
	if ( WM_NOTIFY == message )
	{
		NMHDR* pNmHdr = (NMHDR*)lParam;

		if ( pNmHdr->hwndFrom != nullptr && ( pNmHdr->code == TTN_NEEDTEXTW || pNmHdr->code == TTN_NEEDTEXTA ) )
			if ( OnTtnNeedText( pNmHdr ) )
				return TRUE;
	}

	return __super::WindowProc( message, wParam, lParam );
}

bool CTooltipsHook::OnTtnNeedText( NMHDR* pNmHdr )
{
	if ( ui::CCmdInfoStore::Instance().HandleTooltipNeedText( pNmHdr, nullptr, m_pCustomCmdInfo ) )
		return true;		// message handled

	return false;
}


// CToolTipsHandlerHook implementation

CToolTipsHandlerHook::CToolTipsHandlerHook( ui::IToolTipsHandler* pToolTipsHandler, CToolTipCtrl* pToolTip )
	: CWindowHook( false )
	, m_pToolTipsHandler( pToolTipsHandler )
	, m_pToolTip( pToolTip )
{
	ASSERT_PTR( m_pToolTip->GetSafeHwnd() );
	ASSERT_PTR( m_pToolTipsHandler );
}

CToolTipsHandlerHook::~CToolTipsHandlerHook()
{
}

CWindowHook* CToolTipsHandlerHook::CreateHook( CWnd* pToolTipOwnerWnd, ui::IToolTipsHandler* pToolTipsHandler, CToolTipCtrl* pToolTip )
{
	CToolTipsHandlerHook* pHook = new CToolTipsHandlerHook( pToolTipsHandler, pToolTip );

	pHook->HookWindow( pToolTipOwnerWnd->GetSafeHwnd() );
	return pHook;
}

LRESULT CToolTipsHandlerHook::WindowProc( UINT message, WPARAM wParam, LPARAM lParam ) override
{
	if ( WM_NOTIFY == message )
	{
		NMHDR* pNmHdr = (NMHDR*)lParam;

		if ( TTN_NEEDTEXT == pNmHdr->code && m_pToolTip->GetSafeHwnd() == pNmHdr->hwndFrom )
			if ( OnTtnNeedText( pNmHdr ) )
				return TRUE;
	}

	return __super::WindowProc( message, wParam, lParam );
}

bool CToolTipsHandlerHook::OnTtnNeedText( NMHDR* pNmHdr )
{
	CPoint point = ui::GetCursorPos( m_hWnd );

	return m_pToolTipsHandler->Handle_TtnNeedText( reinterpret_cast<NMTTDISPINFO*>( pNmHdr ), point );
}
