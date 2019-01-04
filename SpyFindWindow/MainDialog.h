#ifndef MainDialog_h
#define MainDialog_h
#pragma once

#include "utl/UI/BaseMainDialog.h"
#include "WndSpot.h"
#include "TrackWndPickerStatic.h"


class CWndHighlighter;


class CMainDialog : public CBaseMainDialog
{
public:
	CMainDialog( void );
	virtual ~CMainDialog();
private:
	CWndSpot m_selWnd;
	std::auto_ptr< CWndHighlighter > m_pHighlighter;
private:
	// enum { IDD = IDD_MAIN_DIALOG };

	CComboBox m_frameStyleCombo;
	CSpinButtonCtrl m_frameSizeSpin;
	CTrackWndPickerStatic m_wndPickerTool;
protected:
	virtual void DoDataExchange( CDataExchange* pDX );
protected:
	// message map functions
	afx_msg void OnTfnTrackMoved( void );
	afx_msg void OnTfnTrackFoundWindow( void );
	afx_msg void OnHighlight( void );
	afx_msg LRESULT OnEndHighlight( WPARAM, LPARAM );
	afx_msg void OnOptionChanged( void );

	DECLARE_MESSAGE_MAP()
};


#endif // MainDialog_h
