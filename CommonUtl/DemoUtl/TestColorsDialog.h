#ifndef TestColorsDialog_h
#define TestColorsDialog_h
#pragma once

#include "utl/UI/LayoutDialog.h"
#include <afxcontrolbars.h>		// MFC support for ribbon and control bars


class CColorPickerButton;


class CTestColorsDialog : public CLayoutDialog
{
public:
	CTestColorsDialog( CWnd* pParent );
	virtual ~CTestColorsDialog();
private:
	COLORREF m_color;
private:
	// enum { IDD = IDD_TEST_COLORS_DIALOG };
	CMFCColorButton m_colorPickerButton;		// drop-down button
	std::auto_ptr<CColorPickerButton> m_pMyColorPicker;

	// generated stuff
protected:
	virtual void DoDataExchange( CDataExchange* pDX );
	virtual void OnCancel( void ) { OnOK(); }
protected:
	afx_msg void OnColorPicker( void );
	afx_msg void OnMyColorPicker( void );

	DECLARE_MESSAGE_MAP()
};


class CMFCColorButton;


namespace ui
{
	void DDX_ColorButton( CDataExchange* pDX, int ctrlId, CMFCColorButton& rCtrl, COLORREF* pColor = nullptr );
	void DDX_ColorText( CDataExchange* pDX, int ctrlId, COLORREF* pColor, bool doInput = false );
	void DDX_ColorRepoText( CDataExchange* pDX, int ctrlId, COLORREF color );
}


#endif // TestColorsDialog_h