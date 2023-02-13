#pragma once

#include "AutomationBase.h"


// CodeProcessor command target

class CodeProcessor : public CCmdTarget
	, private CAutomationBase
{
	DECLARE_DYNCREATE(CodeProcessor)
protected:
	CodeProcessor( void );
	virtual ~CodeProcessor();
private:
	CString m_docLanguage;
	int m_tabSize;
	bool m_useTabs;
public:
	// generated stuff
public:
	virtual void OnFinalRelease( void );
protected:
	DECLARE_MESSAGE_MAP()
	DECLARE_OLECREATE(CodeProcessor)

	DECLARE_DISPATCH_MAP()
	DECLARE_INTERFACE_MAP()

	// generated OLE dispatch map functions
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

	enum 
	{
		dispidDocLanguage = 1,
		dispidTabSize = 2,
		dispidUseTabs = 3,
		dispidCancelTag = 4,
		dispidAutoFormatCode = 5,
		dispidSplitArgumentList = 6,
		dispidExtractTypeDescriptor = 7,
		dispidImplementMethods = 8,
		dispidToggleComment = 9,
		dispidFormatWhitespaces = 10,
		dispidGenerateConsecutiveNumbers = 11,
		dispidSortLines = 12,
		dispidAutoMakeCode = 13,
		dispidTokenizeText = 14
	};
};
