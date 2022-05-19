#if !defined(AFX_CODEFORMATTING_H__C841D2DA_1177_4AF1_A68C_3EA45D9166BC__INCLUDED_)
#define AFX_CODEFORMATTING_H__C841D2DA_1177_4AF1_A68C_3EA45D9166BC__INCLUDED_
#pragma once

#include "AutomationBase.h"

/////////////////////////////////////////////////////////////////////////////
// CCodeProcessor command target

class CCodeProcessor : public CAutomationBase
{
	DECLARE_DYNCREATE(CCodeProcessor)
protected:
	CCodeProcessor( void );
	virtual ~CCodeProcessor();
private:
	CString m_docLanguage;
	int m_tabSize;
	bool m_useTabs;
public:
	// generated overrides
	//{{AFX_VIRTUAL(CCodeProcessor)
	public:
	virtual void OnFinalRelease();
	//}}AFX_VIRTUAL
protected:
	// generated message map
	//{{AFX_MSG(CCodeProcessor)
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
	DECLARE_OLECREATE(CCodeProcessor)

	// Generated OLE dispatch map functions
	//{{AFX_DISPATCH(CCodeProcessor)
	afx_msg void SetDocLanguage(LPCTSTR lpszNewValue);
	afx_msg void SetTabSize(long nNewValue);
	afx_msg void SetUseTabs(BOOL bNewValue);
	afx_msg BSTR GetCancelTag();
	afx_msg BSTR AutoFormatCode(LPCTSTR codeText);
	afx_msg BSTR SplitArgumentList(LPCTSTR codeText, long splitAtColumn, long targetBracketLevel);
	afx_msg BSTR ExtractTypeDescriptor(LPCTSTR functionImplLine, LPCTSTR docFileExt);
	afx_msg BSTR ImplementMethods(LPCTSTR methodPrototypes, LPCTSTR typeDescriptor, BOOL isInline);
	afx_msg BSTR ToggleComment(LPCTSTR codeText);
	afx_msg BSTR FormatWhitespaces(LPCTSTR codeText);
	afx_msg BSTR GenerateConsecutiveNumbers(LPCTSTR codeText);
	afx_msg BSTR SortLines(LPCTSTR codeText, BOOL ascending);
	afx_msg BSTR AutoMakeCode(LPCTSTR codeText);
	afx_msg BSTR TokenizeText(LPCTSTR codeText);
	//}}AFX_DISPATCH
	DECLARE_DISPATCH_MAP()
	DECLARE_INTERFACE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CODEFORMATTING_H__C841D2DA_1177_4AF1_A68C_3EA45D9166BC__INCLUDED_)
