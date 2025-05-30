#pragma once

#include <vector>
#include "utl/AppTools.h"
#include "PathInfo.h"


class DspParser;


class DspProject : public CCmdTarget
	, private app::CLazyInitAppResources
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
	std::vector<PathInfoEx> m_sourceFileFilters;

	std::auto_ptr<DspParser> m_parserPtr;
	std::vector<PathInfoEx> m_projectFiles;
	std::vector<PathInfoEx> m_diskSourceFiles;
	std::vector<PathInfoEx> m_filesToAdd;
	std::vector<PathInfoEx> m_filesToRemove;
public:

	// generated stuff
public:
	virtual void OnFinalRelease();
protected:
	DECLARE_MESSAGE_MAP()
	DECLARE_OLECREATE(DspProject)
	DECLARE_DISPATCH_MAP()
	DECLARE_INTERFACE_MAP()

	// generated OLE dispatch map functions
	enum 
	{
		dispidDspProjectFilePath = 1,
		dispidGetFileFilterCount = 2,
		dispidGetFileFilterAt = 3,
		dispidAddFileFilter = 4,
		dispidClearAllFileFilters = 5,
		dispidGetProjectFileCount = 6,
		dispidGetProjectFileAt = 7,
		dispidContainsSourceFile = 8,
		dispidGetFilesToAddCount = 9,
		dispidGetFileToAddAt = 10,
		dispidGetFilesToRemoveCount = 11,
		dispidGetFileToRemoveAt = 12,
		dispidGetAdditionalIncludePath = 13
	};

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
};
