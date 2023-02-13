#pragma once

#include "AutomationBase.h"
#include "DirPathGroup.h"
#include "ProjectContext.h"


class FileLocator : public CCmdTarget
	, private CAutomationBase
{
	DECLARE_DYNCREATE(FileLocator)
protected:
	FileLocator( void );
	virtual ~FileLocator();
private:
	std::tstring m_selectedFilesFlat;
	std::vector< inc::TPathLocPair > m_selectedFiles;
	CProjectContext m_projCtx;

	// generated stuff
public:
	virtual void OnFinalRelease();
protected:
	DECLARE_MESSAGE_MAP()
	DECLARE_OLECREATE(FileLocator)
	DECLARE_DISPATCH_MAP()
	DECLARE_INTERFACE_MAP()

	// generated OLE dispatch map functions
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

	enum
	{
		dispidLocalDirPath = 1,
		dispidLocalCurrentFile = 2,
		dispidAssociatedProjectFile = 3,
		dispidProjectActiveConfiguration = 4,
		dispidProjectAdditionalIncludePath = 5,
		dispidSelectedFiles = 6,
		dispidSelectedCount = 7,
		dispidGetSelectedFile = 8,
		dispidLocateFile = 9
	};
};
