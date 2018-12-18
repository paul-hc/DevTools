#if !defined(AFX_MENUFILEPICKER_H__4DFA7BE3_8484_11D2_A2C3_006097B8DD84__INCLUDED_)
#define AFX_MENUFILEPICKER_H__4DFA7BE3_8484_11D2_A2C3_006097B8DD84__INCLUDED_
#pragma once

#include "FileBrowser.h"


class MenuFilePicker : public CCmdTarget
{
	DECLARE_DYNCREATE( MenuFilePicker )
protected:
	MenuFilePicker( void );
	virtual ~MenuFilePicker();
public:
	CFileBrowser m_browser;
public:
	//{{AFX_VIRTUAL(MenuFilePicker)
	public:
	virtual void OnFinalRelease( void );
	//}}AFX_VIRTUAL
protected:
	//{{AFX_MSG(MenuFilePicker)
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
	DECLARE_OLECREATE( MenuFilePicker )

	// Generated OLE dispatch map functions
	//{{AFX_DISPATCH(MenuFilePicker)
	long m_trackPosX;
	afx_msg void OnTrackPosXChanged();
	long m_trackPosY;
	afx_msg void OnTrackPosYChanged();
	afx_msg long GetOptionFlags();
	afx_msg void SetOptionFlags( long nNewValue );
	afx_msg long GetFolderLayout();
	afx_msg void SetFolderLayout( long nNewValue );
	afx_msg BSTR GetCurrentFileName();
	afx_msg void SetCurrentFileName( LPCTSTR pSelectedFileName );
	afx_msg BSTR SetProfileSection( LPCTSTR pSubSection, BOOL loadNow );
	afx_msg BOOL AddFolder( LPCTSTR folderPathFilter, LPCTSTR folderAlias );
	afx_msg BOOL AddFolderArray( LPCTSTR folderItemFlatArray );
	afx_msg BOOL AddRootFile( LPCTSTR filePath, LPCTSTR label );
	afx_msg void AddSortOrder( long pathField, BOOL exclusive );
	afx_msg void ClearSortOrder();
	afx_msg void StoreTrackPos();
	afx_msg BOOL ChooseFile();
	afx_msg BOOL OverallExcludeFile( LPCTSTR filePathFilter );
	afx_msg BOOL ExcludeFileFromFolder( LPCTSTR folderPath, LPCTSTR fileFilter );
	//}}AFX_DISPATCH

	DECLARE_DISPATCH_MAP()
	DECLARE_INTERFACE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MENUFILEPICKER_H__4DFA7BE3_8484_11D2_A2C3_006097B8DD84__INCLUDED_)
