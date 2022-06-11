#ifndef ItemContentHistoryCombo_h
#define ItemContentHistoryCombo_h
#pragma once

#include "HistoryComboBox.h"
#include "TandemControls.h"


// history combo for a string, dir-path or file-path with details button

class CItemContentHistoryCombo : public CBaseItemContentCtrl<CHistoryComboBox>
{
	typedef CFrameHostCtrl<CComboBox> TBaseClass;
public:
	CItemContentHistoryCombo( ui::ContentType type = ui::String, const TCHAR* pFileFilter = NULL );
	virtual ~CItemContentHistoryCombo();

	void SelectAll( void ) { SetEditSel( 0, -1 ); }

	// base overrides
	virtual const ui::CItemContent& GetItemContent( void ) const { return m_content; }		// use CBaseItemContentCtrl::m_content rather than CHistoryComboBox::m_itemContent
protected:
	virtual void OnDroppedFiles( const std::vector< fs::CPath >& filePaths );

	// interface IBuddyCommandHandler (may be overridden)
	virtual bool OnBuddyCommand( UINT cmdId );
};


class CSearchPathHistoryCombo : public CItemContentHistoryCombo
{
public:
	CSearchPathHistoryCombo( ui::ContentType type = ui::MixedPath, const TCHAR* pFileFilter = NULL );
private:
	bool m_recurse;
};


#endif // ItemContentHistoryCombo_h
