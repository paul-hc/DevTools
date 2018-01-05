#if !defined(AFX_DSPPROJECT_H__8902DD00_9974_43AF_B8BC_B301EF9F89FE__INCLUDED_)
#define AFX_DSPPROJECT_H__8902DD00_9974_43AF_B8BC_B301EF9F89FE__INCLUDED_
#pragma once

#include <vector>
#include "PathInfo.h"

class DspParser;


class DspProject : public CCmdTarget
{
	DECLARE_DYNCREATE(DspProject)
protected:
	DspProject( void );
	virtual ~DspProject();
private:
	void clear( void );
	void lookupSourceFiles( void );

	bool isSourceFileMatch( const PathInfoEx& filePath ) const;
	size_t filterProjectFiles( void );
private:
	std::vector< PathInfoEx > m_sourceFileFilters;

	std::auto_ptr< DspParser > m_parserPtr;
	std::vector< PathInfoEx > m_projectFiles;
	std::vector< PathInfoEx > m_diskSourceFiles;
	std::vector< PathInfoEx > m_filesToAdd;
	std::vector< PathInfoEx > m_filesToRemove;
public:
// Overrides
	// generated overrides
	//{{AFX_VIRTUAL(DspProject)
	public:
	virtual void OnFinalRelease();
	//}}AFX_VIRTUAL
protected:
	// generated message map
	//{{AFX_MSG(DspProject)
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
	DECLARE_OLECREATE(DspProject)

	// Generated OLE dispatch map functions
	//{{AFX_DISPATCH(DspProject)
	afx_msg BSTR GetDspProjectFilePath();
	afx_msg void SetDspProjectFilePath(LPCTSTR lpszNewValue);
	afx_msg long GetFileFilterCount();
	afx_msg BSTR GetFileFilterAt(long index);
	afx_msg void AddFileFilter(LPCTSTR sourceFileFilter);
	afx_msg void ClearAllFileFilters();
	afx_msg long GetProjectFileCount();
	afx_msg BSTR GetProjectFileAt(long index);
	afx_msg BOOL ContainsSourceFile(LPCTSTR sourceFilePath);
	afx_msg long GetFilesToAddCount();
	afx_msg BSTR GetFileToAddAt(long index);
	afx_msg long GetFilesToRemoveCount();
	afx_msg BSTR GetFileToRemoveAt(long index);
	afx_msg BSTR GetAdditionalIncludePath(LPCTSTR configurationName);
	//}}AFX_DISPATCH

	DECLARE_DISPATCH_MAP()
	DECLARE_INTERFACE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DSPPROJECT_H__8902DD00_9974_43AF_B8BC_B301EF9F89FE__INCLUDED_)
