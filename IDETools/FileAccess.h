#pragma once

#include "utl/AppTools.h"


// FileAccess command target

class FileAccess : public CCmdTarget
	, private app::CLazyInitAppResources
{
	DECLARE_DYNCREATE(FileAccess)
public:
	FileAccess( void );
	virtual ~FileAccess();

	// generated stuff
public:
	virtual void OnFinalRelease();
protected:
	DECLARE_MESSAGE_MAP()
	DECLARE_OLECREATE(FileAccess)
	DECLARE_DISPATCH_MAP()
	DECLARE_INTERFACE_MAP()

	// generated OLE dispatch map functions
	enum
	{
		dispidFileExist = 1,
		dispidGetFileAttributes = 2,
		dispidIsDirectoryFile = 3,
		dispidIsCompressedFile = 4,
		dispidIsReadOnlyFile = 5,
		dispidSetReadOnlyFile = 7,
		dispidExecute = 8,
		dispidShellOpen = 6,
		dispidExploreAndSelectFile = 9,
		dispidGetNextAssocDoc = 10,
		dispidGetComplementaryDoc = 11,
		dispidOutputWndActivateTab = 12,
		dispidGetNextVariationDoc = 13,
		dispidGetIDECurrentBrowseFile = 14,
		dispidUpdateIDECurrentBrowseFile = 15
	};

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
};
