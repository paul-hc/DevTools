#ifndef FileLocatorDialog_h
#define FileLocatorDialog_h
#pragma once

#include "utl/AccelTable.h"
#include "utl/ReportListControl.h"
#include "utl/LayoutDialog.h"
#include "utl/ItemContentEdit.h"
#include "IncludePaths.h"
#include "ProjectContext.h"


class CFileLocatorDialog : public CLayoutDialog
						 , public CProjectContext
{
public:
	CFileLocatorDialog( CWnd* pParent );
	virtual ~CFileLocatorDialog();
private:
	static std::tstring GetAdditionalIncludePaths( void );

	int SearchForTag( const std::tstring& includeTag );
	bool EnableCommandButtons( void );

	void SetComboTag( const std::tstring& includeTag );
	void ModifyTagType( bool localInclude );
	bool isValidLocalDirPath( bool allowEmpty = true );

	void readProfile( void );
	void saveProfile( void );
	void SaveHistory( void );
	void UpdateHistory( const std::tstring& selFile, bool doSave = true );
	bool saveCurrentTagToHistory( void );
private:
	int getCurrentFoundFile( void ) const;
	int getSelectedFoundFiles( std::vector< int >& selFiles );
	std::tstring getSelectedFoundFilesFlat( const std::vector< int >& selFiles, const TCHAR* sep = _T(";") ) const;
	int storeSelection( void );
protected:
	// CProjectContext base overrides
	virtual void OnLocalDirPathChanged( void );
	virtual void OnAssociatedProjectFileChanged( void );
	virtual void OnProjectActiveConfigurationChanged( void );
public:
	int m_tagHistoryMaxCount;
	inc::TSearchFlags m_searchFlags;

	// output properties
	std::tstring m_selectedFilePath;
	std::vector< inc::TPathLocPair > m_selectedFiles;
private:
	std::tstring m_foundFilesFormat;
	std::vector< inc::TPathLocPair > m_foundFiles;
	int m_intrinsic;
	bool m_closedOK;
	std::tstring m_defaultExt;
	CAccelTable m_accelListFocus;
private:
	// enum { IDD = IDD_FILE_LOCATOR_DIALOG };
	CComboBox m_includeTagCombo;
	CItemListEdit m_localDirPathEdit;
	CItemListEdit m_projectFileEdit;
	CReportListControl m_foundFilesListCtrl;

	enum Column { FileName, Directory, Location };

	public:
	virtual BOOL PreTranslateMessage( MSG* pMsg );
	protected:
	virtual void DoDataExchange( CDataExchange* pDX );
protected:
	virtual BOOL OnInitDialog( void );
	virtual void OnOK( void );
	afx_msg void CBnEditChangeInputIncludeTag( void );
	afx_msg void CBnSelChangeInputIncludeTag( void );
	afx_msg void CkSystemTagRadio( void );
	afx_msg void CkLocalTagRadio( void );
	afx_msg void OnEnEditItems_LocalPath( void );
	afx_msg void OnEnEditItems_ProjectFile( void );
	afx_msg HBRUSH OnCtlColor( CDC* pDC, CWnd* pWnd, UINT nCtlColor );
	afx_msg void LVnDblclkFoundFiles( NMHDR* pNmHdr, LRESULT* pResult );
	afx_msg void LVnBeginLabelEditFoundFiles( NMHDR* pNmHdr, LRESULT* pResult );
	afx_msg void LVnItemChangedFoundFiles( NMHDR* pNmHdr, LRESULT* pResult );
	afx_msg void OnDestroy( void );
	afx_msg void OnContextMenu( CWnd* pWnd, CPoint point );
	afx_msg void CmExploreFile( void );
	afx_msg void OnStoreFullPath( void );
	afx_msg void CmViewFile( UINT cmdId );
	afx_msg void CkSearchPath( UINT ckID );

	DECLARE_MESSAGE_MAP()
};


#endif // FileLocatorDialog_h
