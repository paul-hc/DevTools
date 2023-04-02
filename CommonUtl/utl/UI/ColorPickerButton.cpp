
#include "pch.h"
#include "ColorPickerButton.h"
#include "ColorRepository.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CColorPickerButton implementation

CColorPickerButton::CColorPickerButton( void )
	: CMFCColorButton()
	, m_pTableGroup( new CColorTableGroup( *CColorRepository::Instance() ) )
{
}

CColorPickerButton::~CColorPickerButton()
{
}

void CColorPickerButton::QueryTooltipText( std::tstring& rText, UINT cmdId, CToolTipCtrl* pTooltip ) const
{
	cmdId, pTooltip;

	if ( m_pTableGroup.get() != nullptr )
		rText = m_pTableGroup->FormatColorMatch( GetColor() );
}


// CMenuPickerButton implementation

CMenuPickerButton::CMenuPickerButton( void )
	: CMFCMenuButton()
{
}

CMenuPickerButton::~CMenuPickerButton()
{
}


// message handlers
BEGIN_MESSAGE_MAP(CMenuPickerButton, CMFCMenuButton)
	ON_WM_INITMENUPOPUP()
END_MESSAGE_MAP()

void CMenuPickerButton::OnInitMenuPopup( CMenu* pPopupMenu, UINT index, BOOL isSysMenu )
{
	__super::OnInitMenuPopup( pPopupMenu, index, isSysMenu );

	GetParent()->SendMessage( WM_INITMENUPOPUP, (WPARAM)pPopupMenu->GetSafeHmenu(), MAKELPARAM( index, isSysMenu ) );
}
