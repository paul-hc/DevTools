#ifndef TestColorsDialog_h
#define TestColorsDialog_h
#pragma once

#include "utl/UI/LayoutDialog.h"
#include <afxcontrolbars.h>		// MFC support for ribbon and control bars


class CColorPickerButton;
class CMenuPickerButton;


class CTestColorsDialog : public CLayoutDialog
{
public:
	CTestColorsDialog( CWnd* pParent );
	virtual ~CTestColorsDialog();
private:
	void SetPickerUserColors( bool pickerUserColors );
private:
	static COLORREF s_color;		// survives dialog's lifetime
	bool m_editChecked;
	CMenu m_popupMenu;
private:
	// enum { IDD = IDD_TEST_COLORS_DIALOG };
	CMFCColorButton m_mfcColorPickerButton;		// drop-down button

	std::auto_ptr<CColorPickerButton> m_pColorPicker;

	std::auto_ptr<CMenuPickerButton> m_pMenuPicker;

	// generated stuff
protected:
	virtual void DoDataExchange( CDataExchange* pDX );
	virtual void OnCancel( void ) { OnOK(); }
protected:
	afx_msg void OnMfcColorPicker( void );
	afx_msg void OnColorPicker( void );
	afx_msg void OnMenuPicker( void );
	afx_msg void On_EditColor( void );
	afx_msg void OnUpdate_EditItem( CCmdUI* pCmdUI );
	afx_msg void OnToggle_PickerUserColors( void );

	DECLARE_MESSAGE_MAP()
};


#endif // TestColorsDialog_h
