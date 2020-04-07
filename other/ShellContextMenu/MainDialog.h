#pragma once

#include "utl/UI/BaseMainDialog.h"
#include "utl/UI/ReportListControl.h"
#include "utl/UI/ImageEdit.h"
#include "utl/UI/TextEdit.h"


enum ListViewMode { LargeIconView, SmallIconView, ListView, ReportView };

struct CFileItemInfo;


class CMainDialog : public CBaseMainDialog
{
public:
	CMainDialog( void );
	virtual ~CMainDialog();
public:
	ListViewMode GetViewMode( void ) const { return m_listViewMode; }
	void SetListViewMode( ListViewMode listViewMode );

	bool UseCustomMenu( void ) const { return m_useCustomMenu; }
	void SetUseCustomMenu( bool useCustomMenu = true ) { m_useCustomMenu = useCustomMenu; }
private:
	void ReadDirectory( const std::tstring& dirPath );

	void QuerySelectedFilePaths( std::vector< std::tstring >& rSelFilePaths ) const;
	int TrackContextMenu( const std::vector< std::tstring >& selFilePaths, const CPoint& screenPos );
	bool InvokeDefaultVerb( const std::vector< std::tstring >& selFilePaths );

	static void QueryDirectoryItems( std::vector< CFileItemInfo* >& rItems, const std::tstring& dirPath );
private:
	ListViewMode m_listViewMode;
	bool m_useCustomMenu;
	std::tstring m_currDirPath;
	int m_currDirImageIndex;

	std::vector< CFileItemInfo* > m_items;
	static const TCHAR s_rootPath[];
private:
	// enum { IDD = IDD_MAIN_DIALOG };
	enum Column { Filename, Size, Type, LastModified, Attributes };

	CImageEdit m_dirPathEdit;
	CReportListControl m_fileListCtrl;		// virtual list control

	// generated stuff
protected:
	virtual void DoDataExchange( CDataExchange* pDX );
protected:
	afx_msg void OnDestroy( void );
	afx_msg void OnLvnRClick_FileList( NMHDR* pNmHdr, LRESULT* pResult );
	afx_msg void OnLvnDblclk_FileList( NMHDR* pNmHdr, LRESULT* pResult );
	afx_msg void OnGetDispInfoFileList( NMHDR* pNmHdr, LRESULT* pResult );
	afx_msg void OnViewMode( UINT cmdId );
	afx_msg void OnUpdateViewMode( CCmdUI* pCmdUI );
	afx_msg void OnUseCustomMenu( void );
	afx_msg void OnUpdateUseCustomMenu( CCmdUI* pCmdUI );

	DECLARE_MESSAGE_MAP()
};
