#if !defined(AFX_MODULEOPTIONS_H__9F8BBF8E_5AB9_400F_8C52_3CE4487806A7__INCLUDED_)
#define AFX_MODULEOPTIONS_H__9F8BBF8E_5AB9_400F_8C52_3CE4487806A7__INCLUDED_
#pragma once


// This automation object gets created in the VC++ macro initialization via the SetupMacroParameters() function, which is called on script startup.
// Macro file locations example:
//	Visual Studio 6		C:\Program Files (x86)\Microsoft Visual Studio\Common\MSDev98\Macros\C++ Macros.dsm
//	Visual Studio 2008	C:\Users\Paul\Documents\Visual Studio 2008\Projects\VSMacros80\CppMacros\CppMacros.vsmacros


class ModuleOptions : public CCmdTarget
{
	DECLARE_DYNCREATE( ModuleOptions )
protected:
	ModuleOptions();		   // protected constructor used by dynamic creation
	virtual ~ModuleOptions();

// Attributes
public:

public:
// Overrides
	// generated overrides
	//{{AFX_VIRTUAL(ModuleOptions)
	public:
	virtual void OnFinalRelease();
	//}}AFX_VIRTUAL

protected:
	// generated message map
	//{{AFX_MSG(ModuleOptions)
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
	DECLARE_OLECREATE(ModuleOptions)

	// Generated OLE dispatch map functions
	//{{AFX_DISPATCH(ModuleOptions)
	afx_msg BSTR GetDeveloperName();
	afx_msg void SetDeveloperName(LPCTSTR lpszNewValue);
	afx_msg BSTR GetCodeTemplateFile();
	afx_msg void SetCodeTemplateFile(LPCTSTR lpszNewValue);
	afx_msg long GetSplitMaxColumn();
	afx_msg void SetSplitMaxColumn(long nNewValue);
	afx_msg long GetMenuVertSplitCount();
	afx_msg void SetMenuVertSplitCount(long nNewValue);
	afx_msg BSTR GetSingleLineCommentToken();
	afx_msg void SetSingleLineCommentToken(LPCTSTR lpszNewValue);
	afx_msg BSTR GetClassPrefix();
	afx_msg void SetClassPrefix(LPCTSTR lpszNewValue);
	afx_msg BSTR GetStructPrefix();
	afx_msg void SetStructPrefix(LPCTSTR lpszNewValue);
	afx_msg BSTR GetEnumPrefix();
	afx_msg void SetEnumPrefix(LPCTSTR lpszNewValue);
	afx_msg BOOL GetAutoCodeGeneration();
	afx_msg void SetAutoCodeGeneration(BOOL bNewValue);
	afx_msg BOOL GetDisplayErrorMessages();
	afx_msg void SetDisplayErrorMessages(BOOL bNewValue);
	afx_msg BOOL GetUseCommentDecoration();
	afx_msg void SetUseCommentDecoration(BOOL bNewValue);
	afx_msg BOOL GetDuplicateLineMoveDown();
	afx_msg void SetDuplicateLineMoveDown(BOOL bNewValue);
	afx_msg BSTR GetBrowseInfoPath();
	afx_msg void SetBrowseInfoPath(LPCTSTR lpszNewValue);
	afx_msg BSTR GetAdditionalIncludePath();
	afx_msg void SetAdditionalIncludePath(LPCTSTR lpszNewValue);
	afx_msg BSTR GetAdditionalAssocFolders();
	afx_msg void SetAdditionalAssocFolders(LPCTSTR lpszNewValue);
	afx_msg long GetLinesBetweenFunctionImpls();
	afx_msg BOOL GetReturnTypeOnSeparateLine();
	afx_msg BSTR GetVStudioCommonDirPath(BOOL addTrailingSlash);
	afx_msg BSTR GetVStudioMacrosDirPath(BOOL addTrailingSlash);
	afx_msg BSTR GetVStudioVC98DirPath(BOOL addTrailingSlash);
	afx_msg BOOL EditOptions();
	afx_msg void LoadOptions();
	afx_msg void SaveOptions();
	//}}AFX_DISPATCH
	DECLARE_DISPATCH_MAP()
	DECLARE_INTERFACE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MODULEOPTIONS_H__9F8BBF8E_5AB9_400F_8C52_3CE4487806A7__INCLUDED_)
