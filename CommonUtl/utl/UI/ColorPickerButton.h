#ifndef ColorPickerButton_h
#define ColorPickerButton_h
#pragma once

#include <afxcolorbutton.h>
#include "Dialog_fwd.h"


class CColorBatchGroup;


class CColorPickerButton : public CMFCColorButton
	, public ui::ICustomCmdInfo
{
public:
	CColorPickerButton( void );
	virtual ~CColorPickerButton();

	const CColorBatchGroup& GetBatchGroup( void ) const { return *m_pBatchGroup; }
	CColorBatchGroup& RefBatchGroup( void ) { return *m_pBatchGroup; }
protected:
	// ui::ICustomCmdInfo interface
	virtual void QueryTooltipText( std::tstring& rText, UINT cmdId, CToolTipCtrl* pTooltip ) const;
private:
	std::unique_ptr<CColorBatchGroup> m_pBatchGroup;
};


#endif // ColorPickerButton_h
