#ifndef InputPasswordDialog_h
#define InputPasswordDialog_h
#pragma once


class CInputPasswordDialog : public CDialog
{
public:
	CInputPasswordDialog( const TCHAR* pCompoundFilePath, CWnd* pParent = NULL );

	// enum { IDD = IDD_INPUT_PASSWORD_DIALOG };
	std::tstring m_password;
	std::tstring m_protectedFileLabel;
public:
	// generated stuff
	protected:
	virtual void DoDataExchange( CDataExchange* pDX );
protected:
	DECLARE_MESSAGE_MAP()
};


#endif // InputPasswordDialog_h
