#ifndef BaseItemTooltipsCtrl_hxx
#define BaseItemTooltipsCtrl_hxx

#include "ui_fwd.h"
#include "MfcUtilities.h"		// for ui::CTooltipTextMessage


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
	ON_NOTIFY_EX_RANGE( TTN_NEEDTEXT, ui::MinCmdId, ui::MaxCmdId, OnTtnNeedText )
END_MESSAGE_MAP()

template< typename BaseCtrl >
INT_PTR CBaseItemTooltipsCtrl<BaseCtrl>::OnToolHitTest( CPoint point, TOOLINFO* pToolInfo ) const
{
	int itemIndex = GetItemFromPoint( point );

	if ( -1 == itemIndex )
		return __super::OnToolHitTest( point, pToolInfo );

	CRect itemRect = GetItemRectAt( itemIndex );

	SetFlag( pToolInfo->uFlags, TTF_CENTERTIP );
	pToolInfo->rect = itemRect;
	pToolInfo->hwnd = m_hWnd;
	pToolInfo->uId = itemIndex + 1;				// index to row number (0 doesn't work)
	pToolInfo->lpszText = LPSTR_TEXTCALLBACK;	// send TTN_NEEDTEXT notifications dynamically
	pToolInfo->lParam = reinterpret_cast<LPARAM>( this );		// identify this control as source window object
	return pToolInfo->uId;
}

template< typename BaseCtrl >
BOOL CBaseItemTooltipsCtrl<BaseCtrl>::OnTtnNeedText( UINT cmdId, NMHDR* pNmHdr, LRESULT* pResult )
{
	cmdId;
	ui::CTooltipTextMessage message( pNmHdr );

	if ( message.IsValidNotification() )
		if ( this == message.m_pData )
		{
			int itemIndex = message.m_cmdId - 1;	// row number to index

			if ( message.AssignTooltipText( GetItemTooltipTextAt( itemIndex ) ) )
			{
				*pResult = 0;
				return TRUE;	// message was handled, tip-text was stored
			}
		}

	return FALSE;				// not handled
}


#endif // BaseItemTooltipsCtrl_hxx
