#ifndef PasswordDialog_h
#define PasswordDialog_h
#pragma once

#include "LayoutDialog.h"
#include "TextEdit.h"


namespace fs { class CPath; }


class CPasswordDialog : public CLayoutDialog
{
public:
	CPasswordDialog( CWnd* pParentWnd, const fs::CPath* pDocPath = nullptr );

	const std::tstring& GetPassword( void ) const { ASSERT( EditMode == m_mode ); return m_password; }
	void SetPassword( const std::tstring& password ) { ASSERT( EditMode == m_mode ); m_password = password; }

	void SetVerifyPassword( const std::tstring& validPassword ) { m_mode = VerifyMode; m_validPassword = validPassword; }
private:
	void RecreateEditCtrls( void );
private:
	enum Mode { EditMode, VerifyMode } m_mode;
	std::tstring m_documentLabel;
	std::tstring m_password;
	std::tstring m_validPassword;		// VerifyMode only
private:
	// enum { IDD = IDD_PASSWORD_DIALOG };

	CTextEdit m_passwordEdit;
	CTextEdit m_confirmPasswordEdit;

	// generated stuff
protected:
	virtual void DoDataExchange( CDataExchange* pDX );
protected:
	afx_msg void OnUpdateOK( CCmdUI* pCmdUI );
	afx_msg void OnModified( void );
	afx_msg void OnToggleShowPassword( void );

	DECLARE_MESSAGE_MAP()
};


#endif // PasswordDialog_h
