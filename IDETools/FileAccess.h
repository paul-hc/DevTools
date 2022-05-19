#if !defined(AFX_FILEACCESS_H__1556FB26_22DB_11D2_A278_006097B8DD84__INCLUDED_)
#define AFX_FILEACCESS_H__1556FB26_22DB_11D2_A278_006097B8DD84__INCLUDED_
#pragma once

#include "AutomationBase.h"


/////////////////////////////////////////////////////////////////////////////
// FileAccess command target

class FileAccess : public CAutomationBase
{
	DECLARE_DYNCREATE(FileAccess)
public:
	FileAccess( void );
	virtual ~FileAccess();
// Attributes
public:

// Operations
public:

// Overrides
	// generated overrides
	//{{AFX_VIRTUAL(FileAccess)
	public:
	virtual void OnFinalRelease();
	//}}AFX_VIRTUAL

// Implementation
protected:
	// generated message map
	//{{AFX_MSG(FileAccess)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
	DECLARE_OLECREATE(FileAccess)

	// Generated OLE dispatch map functions
	//{{AFX_DISPATCH(FileAccess)
	afx_msg BOOL FileExist(LPCTSTR fullPath);
	afx_msg long GetFileAttributes(LPCTSTR fullPath);
	afx_msg BOOL IsDirectoryFile(LPCTSTR fullPath);
	afx_msg BOOL IsCompressedFile(LPCTSTR fullPath);
	afx_msg BOOL IsReadOnlyFile(LPCTSTR fullPath);
	afx_msg BOOL SetReadOnlyFile(LPCTSTR fullPath, BOOL readOnly);
	afx_msg BOOL Execute(LPCTSTR fullPath);
	afx_msg BOOL ShellOpen(LPCTSTR docFullPath);
	afx_msg BOOL ExploreAndSelectFile(LPCTSTR fileFullPath);
	afx_msg BSTR GetNextAssocDoc(LPCTSTR docFullPath, BOOL forward);
	afx_msg BSTR GetNextVariationDoc(LPCTSTR docFullPath, BOOL forward);
	afx_msg BSTR GetComplementaryDoc(LPCTSTR docFullPath);
	afx_msg BOOL OutputWndActivateTab(LPCTSTR tabCaption);
	afx_msg BSTR GetIDECurrentBrowseFile();
	afx_msg BOOL UpdateIDECurrentBrowseFile(BOOL doItNow);
	//}}AFX_DISPATCH
	DECLARE_DISPATCH_MAP()
	DECLARE_INTERFACE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_FILEACCESS_H__1556FB26_22DB_11D2_A278_006097B8DD84__INCLUDED_)
