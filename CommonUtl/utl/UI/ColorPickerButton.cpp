
#include "pch.h"
#include "ColorPickerButton.h"
#include "ColorRepository.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CColorPickerButton::CColorPickerButton( void )
	: CMFCColorButton()
	, m_pBatchGroup( new CColorBatchGroup( *CColorRepository::Instance() ) )
{
}

CColorPickerButton::~CColorPickerButton()
{
}

void CColorPickerButton::QueryTooltipText( std::tstring& rText, UINT cmdId, CToolTipCtrl* pTooltip ) const
{
	cmdId, pTooltip;

	if ( m_pBatchGroup.get() != nullptr )
		rText = m_pBatchGroup->FormatColorMatch( GetColor() );
}
