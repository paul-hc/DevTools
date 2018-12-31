
#ifndef WkspSaveDialog_h
#define WkspSaveDialog_h
#pragma once

#include "utl/ItemContentEdit.h"
#include "utl/LayoutDialog.h"
#include "FileBrowser.h"


class WorkspaceProfile;


class CWkspSaveDialog : public CLayoutDialog
{
public:
	CWkspSaveDialog( WorkspaceProfile& rWkspProfile, const CString& section, const CString& currProjectName, CWnd* pParent );
	virtual ~CWkspSaveDialog();

	void updateFileContents( void );
protected:
	void setupWindow( void );
	void cleanupWindow( void );

	bool loadExistingProjects( void );
	bool saveFiles( void );
	bool readProjectName( void );
	CFileItem* getListFile( int listIndex ) const;
private:
	CString m_section;
	CString m_currProjectName;
	WorkspaceProfile& m_rWkspProfile;
	CFolderOptions& m_rOptions;
	CFolderItem m_folderItem;
	CMenu m_sortOrderPopup;
private:
	// enum { IDD = IDD_WORKSPACE_SAVE_DIALOG };
	CComboBox m_projectNameCombo;
	CDragListBox m_fileList;
	CItemContentEdit m_fullPathEdit;

	// generated stuff
public:
	virtual BOOL OnCmdMsg( UINT id, int code, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo );
protected:
	virtual void DoDataExchange( CDataExchange* pDX );
	virtual LRESULT WindowProc( UINT message, WPARAM wParam, LPARAM lParam );
protected:
	virtual BOOL OnInitDialog( void );
	afx_msg void OnDestroy( void );
	virtual void OnOK( void );
	afx_msg void OnDropFiles( HDROP hDropInfo );
	afx_msg void CmDeleteProjectEntry( void );
	afx_msg void CmDeleteAllProjects( void );
	afx_msg void CkShowFullPath( void );
	afx_msg void CmSortByFiles( void );
	afx_msg void LBnSelChangeFiles( void );
	bool dragListNotify( UINT listID, DRAGLISTINFO& dragInfo );

	DECLARE_MESSAGE_MAP()
};


#endif // WkspSaveDialog_h
