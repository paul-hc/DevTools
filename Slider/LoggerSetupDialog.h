#ifndef LoggerSetupDialog_h
#define LoggerSetupDialog_h
#pragma once


class CLogger;


class CLoggerSetupDialog : public CDialog
{
public:
	CLoggerSetupDialog( CWnd* pParent = nullptr );
private:
	CLogger* m_pCurrLogger;
public:
	//enum { IDD = IDD_LOGGER_DIALOG };
	CComboBox m_selLogCombo;

	protected:
	virtual void DoDataExchange( CDataExchange* pDX );
protected:
	virtual BOOL OnInitDialog( void );
	virtual void OnOK( void );
	virtual void OnCancel( void );
	afx_msg void OnCBnSelChangeLogger( void );
	afx_msg void OnToggle_LoggerEnabled( void );
	afx_msg void OnToggle_TimestampPrefix( void );
	afx_msg void OnViewLogFileButton( void );
	afx_msg void OnClearLogFileButton( void );

	DECLARE_MESSAGE_MAP()
};


#endif // LoggerSetupDialog_h
