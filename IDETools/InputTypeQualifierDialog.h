#if !defined(AFX_INPUTTYPEQUALIFIERDIALOG_H__DB5E7475_953F_44B4_A1E8_34AC29C6B199__INCLUDED_)
#define AFX_INPUTTYPEQUALIFIERDIALOG_H__DB5E7475_953F_44B4_A1E8_34AC29C6B199__INCLUDED_
#pragma once


class InputTypeQualifierDialog : public CDialog
{
public:
	InputTypeQualifierDialog( const CString& typeQualifier, CWnd* pParent );
public:
	//{{AFX_DATA(InputTypeQualifierDialog)
//	enum { IDD = IDD_INPUT_TYPE_QUALIFIER_DIALOG };
	CString	m_typeQualifier;
	//}}AFX_DATA

	// generated overrides
	//{{AFX_VIRTUAL(InputTypeQualifierDialog)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL
protected:
	// generated message map
	//{{AFX_MSG(InputTypeQualifierDialog)
	virtual void OnOK();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};


//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_INPUTTYPEQUALIFIERDIALOG_H__DB5E7475_953F_44B4_A1E8_34AC29C6B199__INCLUDED_)
