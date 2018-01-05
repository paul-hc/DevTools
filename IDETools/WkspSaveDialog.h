
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
	CWkspSaveDialog( WorkspaceProfile& _wkspProfile, const CString& _section, const CString& _currProjectName,
					CWnd* parent );
	virtual ~CWkspSaveDialog();

	void updateFileContents( void );
protected:
	void setupWindow( void );
	void cleanupWindow( void );

	bool loadExistingProjects( void );
	bool saveFiles( void );
	bool readProjectName( void );
	CMetaFolder::CFile* getListFile( int listIndex ) const;
public:
	CString section;
	CString currProjectName;
	WorkspaceProfile& wkspProfile;
	CFolderOptions& m_rOptions;
	CMetaFolder metaFolder;
	CMenu m_sortOrderPopup;

	bool showFullPath;
private:
	// enum { IDD = IDD_WORKSPACE_SAVE_DIALOG };
	CComboBox projectNameCombo;
	CDragListBox fileList;
	CItemContentEdit m_fullPathEdit;

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
