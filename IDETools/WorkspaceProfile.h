#pragma once

#include "AutomationBase.h"


extern LPCTSTR defaulWkspSection;
extern LPCTSTR defaulProjectName;
extern LPCTSTR sectionWorkspaceDialogs;


class WorkspaceProfile : public CCmdTarget
	, private CAutomationBase
{
	DECLARE_DYNCREATE( WorkspaceProfile )
public:
	WorkspaceProfile( void );
	virtual ~WorkspaceProfile();

	int findFile( LPCTSTR fileFullPath ) const;
	int findProjectName( LPCTSTR projectName ) const;

	CString getFileEntryName( int fileIndex ) const;
public:
	CFolderOptions m_options;
	std::vector< CString > projectNameArray;
	std::vector< CString > fileArray;

	// generated stuff
public:
	virtual void OnFinalRelease();
protected:
	DECLARE_MESSAGE_MAP()
	DECLARE_OLECREATE( WorkspaceProfile )

	DECLARE_DISPATCH_MAP()
	DECLARE_INTERFACE_MAP()
public:
	// generated OLE dispatch map functions
	BOOL m_mustCloseAll;
	afx_msg void OnMustCloseAllChanged();
	afx_msg BOOL AddFile( LPCTSTR fileFullPath );
	afx_msg BOOL AddProjectName( LPCTSTR projectName );
	afx_msg long GetFileCount();
	afx_msg BSTR GetFileName( long index );
	afx_msg BOOL Save( LPCTSTR section, LPCTSTR currProjectName );
	afx_msg BOOL Load( LPCTSTR section, LPCTSTR currProjectName );

	enum
	{
		// properties:
		dispidMustCloseAll = 1,
		// methods:
		dispidAddFile = 2,
		dispidAddProjectName = 3,
		dispidGetFileCount = 4,
		dispidGetFileName = 5,
		dispidSave = 6,
		dispidLoad = 7
	};
};
