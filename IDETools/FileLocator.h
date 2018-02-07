#if !defined(AFX_FILELOCATOR_H__A0580B97_3350_11D5_B5A4_00D0B74ECB52__INCLUDED_)
#define AFX_FILELOCATOR_H__A0580B97_3350_11D5_B5A4_00D0B74ECB52__INCLUDED_
#pragma once

#include "DirPathGroup.h"
#include "ProjectContext.h"


class FileLocator : public CCmdTarget
{
	DECLARE_DYNCREATE(FileLocator)
protected:
	FileLocator( void );
	virtual ~FileLocator();
private:
	std::tstring m_selectedFilesFlat;
	std::vector< inc::TPathLocPair > m_selectedFiles;
	CProjectContext m_projCtx;
public:
// Overrides
	// generated overrides
	//{{AFX_VIRTUAL(FileLocator)
	public:
	virtual void OnFinalRelease();
	//}}AFX_VIRTUAL
protected:
	// generated message map
	//{{AFX_MSG(FileLocator)
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
	DECLARE_OLECREATE(FileLocator)

	// generated OLE dispatch map functions
	//{{AFX_DISPATCH(FileLocator)
	afx_msg BSTR GetLocalDirPath();
	afx_msg void SetLocalDirPath(LPCTSTR lpszNewValue);
	afx_msg BSTR GetLocalCurrentFile();
	afx_msg void SetLocalCurrentFile(LPCTSTR lpszNewValue);
	afx_msg BSTR GetAssociatedProjectFile();
	afx_msg void SetAssociatedProjectFile(LPCTSTR lpszNewValue);
	afx_msg BSTR GetProjectActiveConfiguration();
	afx_msg void SetProjectActiveConfiguration(LPCTSTR lpszNewValue);
	afx_msg BSTR GetProjectAdditionalIncludePath();							// OBSOLETE
	afx_msg void SetProjectAdditionalIncludePath(LPCTSTR lpszNewValue);		// OBSOLETE
	afx_msg BSTR GetSelectedFiles();
	afx_msg long GetSelectedCount();
	afx_msg BSTR GetSelectedFile(long index);
	afx_msg BOOL LocateFile();
	//}}AFX_DISPATCH
	DECLARE_DISPATCH_MAP()
	DECLARE_INTERFACE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_FILELOCATOR_H__A0580B97_3350_11D5_B5A4_00D0B74ECB52__INCLUDED_)
