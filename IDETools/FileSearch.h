#pragma once

#include "utl/AppTools.h"


// FileSearch command target

class FileSearch : public CCmdTarget
	, private app::CLazyInitAppResources
{
	DECLARE_DYNCREATE(FileSearch)

	FileSearch();           // protected constructor used by dynamic creation
	virtual ~FileSearch();
private:
	bool isFiltered( void ) const;
	bool isTargetFile( void ) const;
	bool findNextTargetFile( void );
	CString doFindAllFiles( LPCTSTR filePattern, LPCTSTR separator, long& outFileCount, BOOL recurseSubDirs = FALSE );
private:
	CFileFind m_fileFind;
	BOOL nextFound;
	DWORD fileAttrFilterStrict;		// File must have all the attributes specified here (if any)
	DWORD fileAttrFilterStrictNot;	// File must have no one of the attributes specified here (if any)
	DWORD fileAttrFilterOr;			// File must have at least one of the attributes specified (if any)
	bool excludeDirDots;			// true by default, excludes . and .. directories from the results
private:
	WIN32_FIND_DATA* getFindData( void ) const;
	WIN32_FIND_DATA* getNextData( void ) const;

	// generated stuff
public:
	virtual void OnFinalRelease();
protected:
	DECLARE_MESSAGE_MAP()
	DECLARE_OLECREATE(FileSearch)

	DECLARE_DISPATCH_MAP()
	DECLARE_INTERFACE_MAP()

	// generated OLE dispatch map functions
	enum
	{
		// properties:
		dispidFileAttrFilterStrict = 1,
		dispidFileAttrFilterStrictNot = 2,
		dispidFileAttrFilterOr = 3,
		dispidExcludeDirDots = 4,
		dispidFileAttributes = 5,
		dispidFileName = 6,
		dispidFilePath = 7,
		dispidFileTitle = 8,
		dispidFileURL = 9,
		dispidRoot = 10,
		dispidLength = 11,
		dispidIsDots = 12,
		dispidIsReadOnly = 13,
		dispidIsDirectory = 14,
		dispidIsCompressed = 15,
		dispidIsSystem = 16,
		dispidIsHidden = 17,
		dispidIsTemporary = 18,
		dispidIsNormal = 19,
		dispidIsArchived = 20,

		// methods:
		dispidFindFile = 21,
		dispidFindNextFile = 22,
		dispidFindAllFiles = 23,
		dispidClose = 24,
		dispidMatchesMask = 25,
		dispidBuildSubDirFilePattern = 26,
		dispidSetupForSubDirSearch = 27,
	};

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
};
