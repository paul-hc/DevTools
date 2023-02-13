#pragma once

#include "AutomationBase.h"


class IncludeFileTree : public CCmdTarget
	, private CAutomationBase
{
	DECLARE_DYNCREATE(IncludeFileTree)

	IncludeFileTree( void );
	virtual ~IncludeFileTree();

	// generated stuff
public:
	virtual void OnFinalRelease();
protected:
	DECLARE_MESSAGE_MAP()
	DECLARE_OLECREATE(IncludeFileTree)

	DECLARE_DISPATCH_MAP()
	DECLARE_INTERFACE_MAP()

	// generated OLE dispatch map functions
	CString m_pickedIncludeFile;
	long m_promptLineNo;
	afx_msg void OnPickedIncludeFileChanged();
	afx_msg void OnPromptLineNoChanged();
	afx_msg BOOL BrowseIncludeFiles(LPCTSTR targetFileName);

	enum
	{
		// properties:
		dispidPickedIncludeFile = 1,
		dispidPromptLineNo = 2,

		// methods:
		dispidBrowseIncludeFiles = 3
	};
};
