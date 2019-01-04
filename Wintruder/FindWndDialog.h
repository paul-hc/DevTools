
#ifndef FindWndDialog_h
#define FindWndDialog_h
#pragma once

#include "utl/UI/IconButton.h"
#include "utl/UI/LayoutDialog.h"
#include "utl/UI/PopupSplitButton.h"
#include "TrackWndPickerStatic.h"


struct CWndSearchPattern;


class CFindWndDialog : public CLayoutDialog
{
public:
	CFindWndDialog( CWndSearchPattern* pPattern, CWnd* pParent );
	virtual ~CFindWndDialog();
private:
	CWndSearchPattern* m_pPattern;
private:
	// enum { IDD = IDD_FIND_WND_DIALOG };

	CTrackWndPickerStatic m_trackWndPicker;
	CComboBox m_wndClassCombo;
	CPopupSplitButton m_resetButton;
protected:
	virtual void DoDataExchange( CDataExchange* pDX );
protected:
	virtual void OnCancel( void );
	afx_msg void OnTsnEndTracking_WndPicker( void );
	afx_msg void OnPatternFieldChanged( UINT ctrlId );
	afx_msg void OnResetDefault( void );
	afx_msg void OnCopyTargetWndHandle( void );
	afx_msg void OnUpdateCopyTargetWndHandle( CCmdUI* pCmdUI );
	afx_msg void OnCopyWndClass( void );
	afx_msg void OnUpdateCopyWndClass( CCmdUI* pCmdUI );
	afx_msg void OnCopyWndCaption( void );
	afx_msg void OnUpdateCopyWndCaption( CCmdUI* pCmdUI );
	afx_msg void OnCopyWndIdent( void );
	afx_msg void OnUpdateCopyWndIdent( CCmdUI* pCmdUI );

	DECLARE_MESSAGE_MAP()
};


#endif // FindWndDialog_h
