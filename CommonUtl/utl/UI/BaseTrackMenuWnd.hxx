#ifndef BaseTrackMenuWnd_hxx
#define BaseTrackMenuWnd_hxx

#include "CmdUpdate.h"


// CBaseTrackMenuWnd template code

BEGIN_TEMPLATE_MESSAGE_MAP( CBaseTrackMenuWnd, BaseWndT, BaseWndT )
	ON_WM_INITMENUPOPUP()
END_MESSAGE_MAP()

template< typename BaseWndT >
void CBaseTrackMenuWnd<BaseWndT>::OnInitMenuPopup( CMenu* pPopupMenu, UINT index, BOOL isSysMenu )
{
	ui::HandleInitMenuPopup( this, pPopupMenu, !isSysMenu );
	__super::OnInitMenuPopup( pPopupMenu, index, isSysMenu );
}


#endif // BaseTrackMenuWnd_hxx
