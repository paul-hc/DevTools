#ifndef ItemContentHistoryCombo_h
#define ItemContentHistoryCombo_h
#pragma once

#include "HistoryComboBox.h"
#include "BaseDetailHostCtrl.h"


// history combo for a string, dir-path or file-path with details button

class CItemContentHistoryCombo : public CBaseItemContentCtrl<CHistoryComboBox>
{
public:
	CItemContentHistoryCombo( ui::ContentType type = ui::String, const TCHAR* pFileFilter = NULL );
	virtual ~CItemContentHistoryCombo();

	void SelectAll( void ) { SetEditSel( 0, -1 ); }

	// base overrides
	virtual const ui::CItemContent& GetItemContent( void ) const { return m_content; }		// use CBaseItemContentCtrl::m_content rather than CHistoryComboBox::m_itemContent
protected:
	// interface IBuddyCommand (may be overridden)
	virtual void OnBuddyCommand( UINT cmdId );
};


#endif // ItemContentHistoryCombo_h
