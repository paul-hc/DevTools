#if !defined(AFX_USERINTERFACE_H__216EF196_4C10_11D3_A3C8_006097B8DD84__INCLUDED_)
#define AFX_USERINTERFACE_H__216EF196_4C10_11D3_A3C8_006097B8DD84__INCLUDED_
#pragma once

/////////////////////////////////////////////////////////////////////////////
// UserInterface command target

class UserInterface : public CCmdTarget
{
	DECLARE_DYNCREATE(UserInterface)

	UserInterface();           // protected constructor used by dynamic creation

// Attributes
public:

// Operations
public:
// Overrides
	// generated overrides
	//{{AFX_VIRTUAL(UserInterface)
	public:
	virtual void OnFinalRelease();
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~UserInterface();

	// generated message map
	//{{AFX_MSG(UserInterface)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
	DECLARE_OLECREATE(UserInterface)

	// Generated OLE dispatch map functions
	//{{AFX_DISPATCH(UserInterface)
	afx_msg BSTR GetIDEToolsRegistryKey();
	afx_msg void SetIDEToolsRegistryKey(LPCTSTR lpszNewValue);
	afx_msg BSTR InputBox(LPCTSTR title, LPCTSTR prompt, LPCTSTR initialValue);
	afx_msg BSTR GetClipboardText();
	afx_msg void SetClipboardText(LPCTSTR text);
	afx_msg BOOL IsClipFormatAvailable(long clipFormat);
	afx_msg BOOL IsClipFormatNameAvailable(LPCTSTR formatName);
	afx_msg BOOL IsKeyPath(LPCTSTR keyFullPath);
	afx_msg BOOL CreateKeyPath(LPCTSTR keyFullPath);
	afx_msg BSTR RegReadString(LPCTSTR keyFullPath, LPCTSTR valueName, LPCTSTR defaultString);
	afx_msg long RegReadNumber(LPCTSTR keyFullPath, LPCTSTR valueName, long defaultNumber);
	afx_msg BOOL RegWriteString(LPCTSTR keyFullPath, LPCTSTR valueName, LPCTSTR strValue);
	afx_msg BOOL RegWriteNumber(LPCTSTR keyFullPath, LPCTSTR valueName, long numValue);
	afx_msg BOOL EnsureStringValue(LPCTSTR keyFullPath, LPCTSTR valueName, LPCTSTR defaultString);
	afx_msg BOOL EnsureNumberValue(LPCTSTR keyFullPath, LPCTSTR valueName, long defaultNumber);
	afx_msg BSTR GetEnvironmentVariable(LPCTSTR varName);
	afx_msg BOOL SetEnvironmentVariable(LPCTSTR varName, LPCTSTR varValue);
	afx_msg BSTR ExpandEnvironmentVariables(LPCTSTR sourceString);
	afx_msg BSTR LocateFile(LPCTSTR localDirPath);
	//}}AFX_DISPATCH

	DECLARE_DISPATCH_MAP()
	DECLARE_INTERFACE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_USERINTERFACE_H__216EF196_4C10_11D3_A3C8_006097B8DD84__INCLUDED_)
