#ifndef ColorPickerButton_h
#define ColorPickerButton_h
#pragma once

#include <afxcolorbutton.h>
#include <afxmenubutton.h>
#include "Dialog_fwd.h"


class CColorTableGroup;


class CColorPickerButton : public CMFCColorButton
	, public ui::ICustomCmdInfo
{
public:
	CColorPickerButton( void );
	virtual ~CColorPickerButton();

	const CColorTableGroup& GetTableGroup( void ) const { return *m_pTableGroup; }
	CColorTableGroup& RefTableGroup( void ) { return *m_pTableGroup; }
protected:
	// ui::ICustomCmdInfo interface
	virtual void QueryTooltipText( std::tstring& rText, UINT cmdId, CToolTipCtrl* pTooltip ) const;
private:
	std::unique_ptr<CColorTableGroup> m_pTableGroup;
};


class CMenuPickerButton : public CMFCMenuButton
{
public:
	CMenuPickerButton( void );
	virtual ~CMenuPickerButton();
protected:
	afx_msg void OnInitMenuPopup( CMenu* pPopupMenu, UINT index, BOOL isSysMenu );

	DECLARE_MESSAGE_MAP()
};


#endif // ColorPickerButton_h
