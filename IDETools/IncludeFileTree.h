#pragma once

#include "utl/AppTools.h"


class IncludeFileTree : public CCmdTarget
	, private app::CLazyInitAppResources
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
	enum
	{
		// properties:
		dispidPickedIncludeFile = 1,
		dispidPromptLineNo = 2,

		// methods:
		dispidBrowseIncludeFiles = 3
	};

	CString m_pickedIncludeFile;
	long m_promptLineNo;
	afx_msg void OnPickedIncludeFileChanged();
	afx_msg void OnPromptLineNoChanged();
	afx_msg BOOL BrowseIncludeFiles(LPCTSTR targetFileName);
};
