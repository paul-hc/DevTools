#if !defined(AFX_INCLUDEFILETREE_H__1006E3E8_1F6F_11D2_A275_006097B8DD84__INCLUDED_)
#define AFX_INCLUDEFILETREE_H__1006E3E8_1F6F_11D2_A275_006097B8DD84__INCLUDED_
#pragma once


class IncludeFileTree : public CCmdTarget
{
	DECLARE_DYNCREATE(IncludeFileTree)

	IncludeFileTree( void );
protected:
	virtual ~IncludeFileTree();
protected:
public:
	//{{AFX_VIRTUAL(IncludeFileTree)
	public:
	virtual void OnFinalRelease();
	//}}AFX_VIRTUAL
protected:
	// generated message map
	//{{AFX_MSG(IncludeFileTree)
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
	DECLARE_OLECREATE(IncludeFileTree)

	// Generated OLE dispatch map functions
	//{{AFX_DISPATCH(IncludeFileTree)
	CString m_pickedIncludeFile;
	long m_promptLineNo;
	afx_msg void OnPickedIncludeFileChanged();
	afx_msg void OnPromptLineNoChanged();
	afx_msg BOOL BrowseIncludeFiles(LPCTSTR targetFileName);
	//}}AFX_DISPATCH
	DECLARE_DISPATCH_MAP()
	DECLARE_INTERFACE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_INCLUDEFILETREE_H__1006E3E8_1F6F_11D2_A275_006097B8DD84__INCLUDED_)
