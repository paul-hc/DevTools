#ifndef CodeMessageBox_h
#define CodeMessageBox_h
#pragma once

#include "utl/UI/LayoutDialog.h"
#include "utl/UI/TextEdit.h"
#include "IdeUtilities.h"		// convenient for using ide::CScopedWindow


class CCodeMessageBox : public CLayoutDialog
{
public:
	CCodeMessageBox( const std::tstring& message, const std::tstring& codeText, UINT mbType = MB_ICONQUESTION, CWnd* pParent = nullptr );
	virtual ~CCodeMessageBox();
public:
	std::tstring m_caption;
private:
	std::tstring m_message;
	std::tstring m_codeText;
	UINT m_mbType;

	CFont messageFont;
private:
	// enum { IDD = IDD_CODE_MESSAGE_BOX_DIALOG };

	CStatic m_iconStatic;
	CStatic m_messageStatic;
	CTextEdit m_codeEdit;
public:
	// generated stuff
protected:
	virtual void DoDataExchange( CDataExchange* pDX );
protected:
	virtual BOOL OnInitDialog( void );
	afx_msg HBRUSH OnCtlColor( CDC* pDC, CWnd* pWnd, UINT ctlColorType );

	DECLARE_MESSAGE_MAP()
};


#endif // CodeMessageBox_h
