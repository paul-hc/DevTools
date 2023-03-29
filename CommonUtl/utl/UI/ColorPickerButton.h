#ifndef ColorPickerButton_h
#define ColorPickerButton_h
#pragma once

#include <afxcolorbutton.h>
#include "Dialog_fwd.h"


class CColorPickerButton : public CMFCColorButton
	, public ui::ICustomCmdInfo
{
public:
	CColorPickerButton( void );
	virtual ~CColorPickerButton();
protected:
	// ui::ICustomCmdInfo interface
	virtual void QueryTooltipText( std::tstring& rText, UINT cmdId, CToolTipCtrl* pTooltip ) const;
private:
};


#endif // ColorPickerButton_h
