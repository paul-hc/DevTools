#ifndef CodeMessageBox_h
#define CodeMessageBox_h
#pragma once

#include "utl/LayoutDialog.h"
#include "utl/TextEdit.h"


class CCodeMessageBox : public CLayoutDialog
{
public:
	CCodeMessageBox( const CString& message, const CString& codeText,
					 UINT mbType = MB_ICONQUESTION,
					 const CString& caption = AfxGetApp()->m_pszProfileName,
					 CWnd* pParent = NULL );
	virtual ~CCodeMessageBox();
private:
	CString m_caption;
	CString m_message;
	CString m_codeText;
	UINT m_mbType;

	CFont messageFont;
private:
	// enum { IDD = IDD_CODE_MESSAGE_BOX_DIALOG };
	CStatic m_iconStatic;
	CStatic m_messageStatic;
	CTextEdit m_codeEdit;
public:
	// generated overrides
	protected:
	virtual void DoDataExchange( CDataExchange* pDX );
protected:
	// generated m_message map
	virtual BOOL OnInitDialog();
	afx_msg HBRUSH OnCtlColor( CDC* pDC, CWnd* pWnd, UINT ctlColorType );
	DECLARE_MESSAGE_MAP()
};


#endif // CodeMessageBox_h
