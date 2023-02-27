#ifndef InputBoxDialog_h
#define InputBoxDialog_h
#pragma once

#include "utl/UI/LayoutDialog.h"


class CInputBoxDialog : public CLayoutDialog
{
public:
	CInputBoxDialog( const TCHAR* pTitle = nullptr, const TCHAR* pPrompt = nullptr, const TCHAR* pInputText = nullptr, CWnd* parent = nullptr );
	virtual ~CInputBoxDialog();
public:
	const TCHAR* m_pTitle;
	const TCHAR* m_pPrompt;
	CString m_inputText;
public:
	// enum { IDD = IDD_INPUT_BOX_DIALOG };
	CEdit inputEdit;
public:
	protected:
	virtual void DoDataExchange( CDataExchange* pDX );
protected:
	DECLARE_MESSAGE_MAP()
};


#endif // InputBoxDialog_h
