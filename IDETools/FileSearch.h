#if !defined(AFX_FILEFIND_H__C722D0B7_1E2D_11D5_B59B_00D0B74ECB52__INCLUDED_)
#define AFX_FILEFIND_H__C722D0B7_1E2D_11D5_B59B_00D0B74ECB52__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// FileSearch.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// FileSearch command target

class FileSearch : public CCmdTarget
{
	DECLARE_DYNCREATE(FileSearch)

	FileSearch();           // protected constructor used by dynamic creation

// Attributes
private:
			bool			isFiltered( void ) const;
			bool			isTargetFile( void ) const;
			bool			findNextTargetFile( void );
			CString			doFindAllFiles( LPCTSTR filePattern, LPCTSTR separator, long& outFileCount,
											BOOL recurseSubDirs = FALSE );
private:
			CFileFind		m_fileFind;
			BOOL			nextFound;
			DWORD			fileAttrFilterStrict;		// File must have all the attributes specified here (if any)
			DWORD			fileAttrFilterStrictNot;	// File must have no one of the attributes specified here (if any)
			DWORD			fileAttrFilterOr;			// File must have at least one of the attributes specified (if any)
			bool			excludeDirDots;				// true by default, excludes . and .. directories from the results
private:
			WIN32_FIND_DATA* getFindData( void ) const;
			WIN32_FIND_DATA* getNextData( void ) const;
// Operations
public:

// Overrides
	// generated overrides
	//{{AFX_VIRTUAL(FileSearch)
	public:
	virtual void OnFinalRelease();
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~FileSearch();

	// generated message map
	//{{AFX_MSG(FileSearch)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
	DECLARE_OLECREATE(FileSearch)

	// Generated OLE dispatch map functions
	//{{AFX_DISPATCH(FileSearch)
	afx_msg long GetFileAttrFilterStrict();
	afx_msg void SetFileAttrFilterStrict(long nNewValue);
	afx_msg long GetFileAttrFilterStrictNot();
	afx_msg void SetFileAttrFilterStrictNot(long nNewValue);
	afx_msg long GetFileAttrFilterOr();
	afx_msg void SetFileAttrFilterOr(long nNewValue);
	afx_msg BOOL GetExcludeDirDots();
	afx_msg void SetExcludeDirDots(BOOL bNewValue);
	afx_msg long GetFileAttributes();
	afx_msg BSTR GetFileName();
	afx_msg BSTR GetFilePath();
	afx_msg BSTR GetFileTitle();
	afx_msg BSTR GetFileURL();
	afx_msg BSTR GetRoot();
	afx_msg long GetLength();
	afx_msg BOOL GetIsDots();
	afx_msg BOOL GetIsReadOnly();
	afx_msg BOOL GetIsDirectory();
	afx_msg BOOL GetIsCompressed();
	afx_msg BOOL GetIsSystem();
	afx_msg BOOL GetIsHidden();
	afx_msg BOOL GetIsTemporary();
	afx_msg BOOL GetIsNormal();
	afx_msg BOOL GetIsArchived();
	afx_msg BOOL FindFile(LPCTSTR filePattern);
	afx_msg BOOL FindNextFile();
	afx_msg BSTR FindAllFiles(LPCTSTR filePattern, LPCTSTR separator, long FAR* outFileCount, BOOL recurseSubDirs);
	afx_msg void Close();
	afx_msg BOOL MatchesMask(long mask);
	afx_msg BSTR BuildSubDirFilePattern(LPCTSTR filePattern);
	afx_msg BSTR SetupForSubDirSearch(LPCTSTR parentFilePattern);
	//}}AFX_DISPATCH
	DECLARE_DISPATCH_MAP()
	DECLARE_INTERFACE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_FILEFIND_H__C722D0B7_1E2D_11D5_B59B_00D0B74ECB52__INCLUDED_)
