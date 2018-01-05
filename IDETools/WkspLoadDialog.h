
#ifndef WkspLoadDialog_h
#define WkspLoadDialog_h
#pragma once

#include "utl/ItemContentEdit.h"
#include "utl/LayoutDialog.h"
#include "FileBrowser.h"


extern LPCTSTR defaulWkspSection;
extern LPCTSTR defaulProjectName;


class WorkspaceProfile;

class CWkspLoadDialog : public CLayoutDialog
{
public:
	CWkspLoadDialog( WorkspaceProfile& _wkspProfile, const CString& _section, const CString& _currProjectName, CWnd* parent );
	virtual ~CWkspLoadDialog();
protected:
	void setupWindow( void );
	void cleanupWindow( void );
	void updateFileContents( void );

	bool loadExistingProjects( void );
	bool transferFiles( void );
	bool readProjectName( void );
	CMetaFolder::CFile* getListFile( int listIndex ) const;

	void handleSelection( Ternary operation );
protected:
	CString section, currProjectName;
	WorkspaceProfile& wkspProfile;
	CFolderOptions options;
	CMetaFolder metaFolder;

	bool showFullPath;
private:
	// enum { IDD = IDD_WORKSPACE_LOAD_DIALOG };
	CComboBox projectNameCombo;
	CListBox fileList;
	CItemContentEdit m_fullPathEdit;

	protected:
	virtual void DoDataExchange( CDataExchange* pDX );
protected:
	virtual BOOL OnInitDialog( void );
	afx_msg void OnDestroy( void );
	afx_msg void OnContextMenu( CWnd* pWnd, CPoint point );
	virtual void OnOK( void );
	afx_msg void CkShowFullPath( void );
	afx_msg void CkCloseAllBeforeOpen( void );
	afx_msg void CBnSelChangeProjectName( void );
	afx_msg void LBnSelChangeFiles( void );
	afx_msg void CmMultiSelection( UINT cmdId );

	DECLARE_MESSAGE_MAP()
};


#endif // WkspLoadDialog_h
