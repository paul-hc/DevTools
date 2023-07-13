#ifndef WorkspaceDialog_h
#define WorkspaceDialog_h
#pragma once

#include "utl/UI/LayoutDialog.h"
#include "utl/UI/SpinEdit.h"
#include "utl/UI/StockValuesComboBox.h"
#include "Workspace.h"


class CColorPickerButton;


class CWorkspaceDialog : public CLayoutDialog
{
public:
	CWorkspaceDialog( CWnd* pParent = nullptr );
	virtual ~CWorkspaceDialog();
public:
	CWorkspaceData m_data;
	int m_thumbnailerFlags;
	bool m_smoothingMode;
	UINT m_defaultSlideDelay;			// in miliseconds
private:
	// enum { IDD = IDD_WORKSPACE_DIALOG };

	CComboBox m_imageScalingCombo;
	CSpinEdit m_mruCountEdit;
	CSpinEdit m_thumbListColumnCountEdit;
	CComboBox m_thumbBoundsSizeCombo;
	CDurationComboBox m_slideDelayCombo;
	std::auto_ptr<CColorPickerButton> m_pDefBkColorPicker;
	std::auto_ptr<CColorPickerButton> m_pImageSelColorPicker;
	std::auto_ptr<CColorPickerButton> m_pImageSelTextColorPicker;

	// generated stuff
protected:
	virtual void DoDataExchange( CDataExchange* pDX );			// DDX/DDV support
protected:
	afx_msg void On_EditBkColor( void );
	afx_msg void OnSaveAndClose( void );
	afx_msg void OnToggle_UseThemedThumbListDraw( void );
	afx_msg void CmEditImageSelColor( void );
	afx_msg void CmEditImageSelTextColor( void );
	afx_msg void CmClearThumbCache( void );

	DECLARE_MESSAGE_MAP()
};


#endif // WorkspaceDialog_h
