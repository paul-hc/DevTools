#ifndef WkspLoadDialog_h
#define WkspLoadDialog_h
#pragma once

#include "utl/UI/ItemContentEdit.h"
#include "utl/UI/LayoutDialog.h"
#include "FileBrowser.h"


extern LPCTSTR defaulWkspSection;
extern LPCTSTR defaulProjectName;


class WorkspaceProfile;

class CWkspLoadDialog : public CLayoutDialog
{
public:
	CWkspLoadDialog( WorkspaceProfile& rWkspProfile, const CString& section, const CString& currProjectName, CWnd* pParent );
	virtual ~CWkspLoadDialog();
private:
	void setupWindow( void );
	void cleanupWindow( void );
	void updateFileContents( void );

	bool loadExistingProjects( void );
	bool transferFiles( void );
	bool readProjectName( void );
	CFileItem* GetListFileItem( int listIndex ) const { return (CFileItem*)m_fileList.GetItemDataPtr( listIndex ); }

	void handleSelection( Ternary operation );
private:
	CString m_section, m_currProjectName;
	WorkspaceProfile& m_rWkspProfile;
	CFolderOptions m_options;
	CFolderItem m_folderItem;
private:
	// enum { IDD = IDD_WORKSPACE_LOAD_DIALOG };
	CComboBox m_projectNameCombo;
	CListBox m_fileList;
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
