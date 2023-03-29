
#include "stdafx.h"
#include "ColorPickerButton.h"
#include "ColorRepository.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CColorPickerButton::CColorPickerButton( void )
	: CMFCColorButton()
{
}

CColorPickerButton::~CColorPickerButton()
{
}

void CColorPickerButton::QueryTooltipText( std::tstring& rText, UINT cmdId, CToolTipCtrl* pTooltip ) const
{
	pTooltip;
	if ( (int)cmdId == GetDlgCtrlID() )
		rText = CColorRepository::Instance()->FormatColorMatch( GetColor() );
}
