
#include "pch.h"
#include "MenuPickerButton.h"
#include "CmdUpdate.h"
#include "MenuUtilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CMenuPickerButton implementation

CMenuPickerButton::CMenuPickerButton( CWnd* pTargetWnd /*= nullptr*/ )
	: CMFCMenuButton()
	, m_pTargetWnd( pTargetWnd )
{
	m_bOSMenu = !ui::UseMfcMenuManager();

	if ( nullptr == m_pTargetWnd )
		m_pTargetWnd = this;
}

CMenuPickerButton::~CMenuPickerButton()
{
}

CWnd* CMenuPickerButton::GetTargetWnd( void ) const
{
	return m_pTargetWnd != nullptr ? m_pTargetWnd : const_cast<CMenuPickerButton*>( this );
}

void CMenuPickerButton::OnShowMenu( void ) override
{
	__super::OnShowMenu();
}

// message handlers

BEGIN_MESSAGE_MAP( CMenuPickerButton, CMFCMenuButton )
	ON_WM_INITMENUPOPUP()
END_MESSAGE_MAP()

void CMenuPickerButton::OnInitMenuPopup( CMenu* pPopupMenu, UINT index, BOOL isSysMenu )
{
	__super::OnInitMenuPopup( pPopupMenu, index, isSysMenu );

	ui::HandleInitMenuPopup( GetTargetWnd(), pPopupMenu, !isSysMenu );		// dialog implements the CmdUI updates
}
