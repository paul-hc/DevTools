
#include "stdafx.h"
#include "ItemContentHistoryCombo.h"
#include "Utilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CItemContentHistoryCombo::CItemContentHistoryCombo( ui::ContentType type /*= ui::String*/, const TCHAR* pFileFilter /*= NULL*/ )
	: CBaseItemContentCtrl< CHistoryComboBox >( type, pFileFilter )
{
	SetContentType( type );					// also switch the icon

	if ( ui::String == type )
		SetStringContent( true, true );		// by default allow a single empty item, no details button
}

CItemContentHistoryCombo::~CItemContentHistoryCombo()
{
}

void CItemContentHistoryCombo::OnBuddyCommand( UINT cmdId )
{
	if ( ui::String == m_content.m_type )					// not very useful
	{
		BaseClass::OnBuddyCommand( cmdId );
		return;
	}
	else
	{
		std::tstring newItem = m_content.EditItem( GetCurrentText().c_str(), GetParent(), cmdId );
		if ( newItem.empty() )
			return;					// cancelled by user

		ui::SetComboEditText( *this, newItem );
		StoreCurrentEditItem();		// store edit text in combo list (with validation)
	}

	SetFocus();
	SelectAll();
	ui::SendCommandToParent( m_hWnd, CN_DETAILSCHANGED );
}
