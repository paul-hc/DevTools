#if !defined(AFX_WORKSPACEPROFILE_H__0E44AB08_90E1_11D2_A2C9_006097B8DD84__INCLUDED_)
#define AFX_WORKSPACEPROFILE_H__0E44AB08_90E1_11D2_A2C9_006097B8DD84__INCLUDED_
#pragma once

#include "AutomationBase.h"


extern LPCTSTR defaulWkspSection;
extern LPCTSTR defaulProjectName;
extern LPCTSTR sectionWorkspaceDialogs;


class WorkspaceProfile : public CAutomationBase
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
public:
	//{{AFX_VIRTUAL(WorkspaceProfile)
	public:
	virtual void OnFinalRelease();
	//}}AFX_VIRTUAL
protected:
	//{{AFX_MSG(WorkspaceProfile)
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
	DECLARE_OLECREATE( WorkspaceProfile )
public:
	//{{AFX_DISPATCH(WorkspaceProfile)
	BOOL m_mustCloseAll;
	afx_msg void OnMustCloseAllChanged();
	afx_msg BOOL AddFile( LPCTSTR fileFullPath );
	afx_msg BOOL AddProjectName( LPCTSTR projectName );
	afx_msg long GetFileCount();
	afx_msg BSTR GetFileName( long index );
	afx_msg BOOL Save( LPCTSTR section, LPCTSTR currProjectName );
	afx_msg BOOL Load( LPCTSTR section, LPCTSTR currProjectName );
	//}}AFX_DISPATCH

	DECLARE_DISPATCH_MAP()
	DECLARE_INTERFACE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WORKSPACEPROFILE_H__0E44AB08_90E1_11D2_A2C9_006097B8DD84__INCLUDED_)
