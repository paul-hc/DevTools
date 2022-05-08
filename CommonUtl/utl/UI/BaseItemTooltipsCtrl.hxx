#ifndef BaseItemTooltipsCtrl_hxx
#define BaseItemTooltipsCtrl_hxx

#include "ui_fwd.h"
#include "CmdInfoStore.h"


// CBaseItemTooltipsCtrl template code

template< typename BaseCtrl >
CBaseItemTooltipsCtrl<BaseCtrl>::CBaseItemTooltipsCtrl( void )
	: BaseCtrl()
{
}

template< typename BaseCtrl >
void CBaseItemTooltipsCtrl<BaseCtrl>::PreSubclassWindow( void )
{
	__super::PreSubclassWindow();

	EnableToolTips( TRUE );
}


// message handlers

BEGIN_TEMPLATE_MESSAGE_MAP( CBaseItemTooltipsCtrl, BaseCtrl, TBaseClass )
	ON_NOTIFY_EX_RANGE( TTN_NEEDTEXTW, ui::MinCmdId, ui::MaxCmdId, OnTtnNeedText )
	ON_NOTIFY_EX_RANGE( TTN_NEEDTEXTA, ui::MinCmdId, ui::MaxCmdId, OnTtnNeedText )
END_MESSAGE_MAP()

template< typename BaseCtrl >
INT_PTR CBaseItemTooltipsCtrl<BaseCtrl>::OnToolHitTest( CPoint point, TOOLINFO* pToolInfo ) const
{
	int itemIndex = GetItemFromPoint( point );

	if ( -1 == itemIndex )
		return __super::OnToolHitTest( point, pToolInfo );

	CRect itemRect = GetItemRectAt( itemIndex );

	pToolInfo->rect = itemRect;
	pToolInfo->hwnd = m_hWnd;
	pToolInfo->uId = itemIndex + 1;					// index to row number (0 doesn't work)
	pToolInfo->lpszText = LPSTR_TEXTCALLBACK;		// send a TTN_NEEDTEXT notifications
	return pToolInfo->uId;
}

template< typename BaseCtrl >
BOOL CBaseItemTooltipsCtrl<BaseCtrl>::OnTtnNeedText( UINT cmdId, NMHDR* pNmHdr, LRESULT* pResult )
{
	cmdId;
	ui::CTooltipTextMessage message( pNmHdr );

	if ( !message.IsValidNotification() )
		return FALSE;		// not handled

	int itemIndex = message.m_cmdId - 1;			// row number to index

	if ( !message.AssignTooltipText( GetItemTooltipTextAt( itemIndex ) ) )
		return FALSE;

	*pResult = 0;
	return TRUE;			// message was handled
}


#endif // BaseItemTooltipsCtrl_hxx
