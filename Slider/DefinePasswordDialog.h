#ifndef DefinePasswordDialog_h
#define DefinePasswordDialog_h
#pragma once


class CDefinePasswordDialog : public CDialog
{
public:
	CDefinePasswordDialog( const TCHAR* pStgDocPath, CWnd* pParent = NULL );

	bool Run( void );
public:
	// enum { IDD = IDD_DEFINE_PASSWORD_DIALOG };

	std::tstring m_password;
	std::tstring m_confirmPassword;
	std::tstring m_protectedFileLabel;
public:
	// generated stuff
	protected:
	virtual void DoDataExchange( CDataExchange* pDX );
protected:
	DECLARE_MESSAGE_MAP()
};


#endif // DefinePasswordDialog_h
