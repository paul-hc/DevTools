#ifndef ColorPickerButton_h
#define ColorPickerButton_h
#pragma once

#include <afxcolorbutton.h>
#include <afxmenubutton.h>
#include "Dialog_fwd.h"
#include "StdColors.h"


class CColorTable;
class CColorStore;


class CColorPickerButton : public CMFCColorButton
	, public ui::ICustomCmdInfo
{
public:
	CColorPickerButton( ui::StdColorTable tableType = ui::Standard_Colors );
	virtual ~CColorPickerButton();

	const CColorStore& GetColorStore( void ) const { return *m_pColorStore; }

	void StoreColors( const std::vector<COLORREF>& colors );
	void SetHalftoneColors( size_t size = 256 );
	void StoreColorTable( const CColorTable* pColorTable );
	void StoreColorTable( ui::StdColorTable tableType );
private:
	static void RegisterColorNames( const CColorTable* pColorTable );
protected:
	// ui::ICustomCmdInfo interface
	virtual void QueryTooltipText( std::tstring& rText, UINT cmdId, CToolTipCtrl* pTooltip ) const override;

	// base overrides:
protected:
	virtual void OnShowColorPopup( void ) override;
private:
	std::unique_ptr<CColorStore> m_pColorStore;
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
