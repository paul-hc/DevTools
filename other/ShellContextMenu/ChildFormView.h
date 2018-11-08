#pragma once

#include "utl/LayoutFormView.h"
#include "utl/ReportListControl.h"


enum ListViewMode { LargeIconView, SmallIconView, ListView, ReportView };

struct CFileItemInfo;


class CChildFormView : public CLayoutFormView
{
	DECLARE_DYNCREATE( CChildFormView )
public:
	CChildFormView( void );
	virtual ~CChildFormView();
public:
	ListViewMode GetViewMode( void ) const { return m_listViewMode; }
	void SetListViewMode( ListViewMode listViewMode );

	bool UseCustomMenu( void ) const { return m_useCustomMenu; }
	void SetUseCustomMenu( bool useCustomMenu = true ) { m_useCustomMenu = useCustomMenu; }
private:
	void ReadDirectory( const std::tstring& dirPath );

	void QuerySelectedFilePaths( std::vector< std::tstring >& rSelFilePaths ) const;
	int TrackContextMenu( const std::vector< std::tstring >& selFilePaths, const CPoint& screenPos );

	static void QueryDirectoryItems( std::vector< CFileItemInfo* >& rItems, const std::tstring& dirPath );
private:
	ListViewMode m_listViewMode;
	bool m_useCustomMenu;
	std::tstring m_currDirPath;

	std::vector< CFileItemInfo* > m_items;
	static const TCHAR s_rootPath[];
private:
	// enum { IDD = IDD_CHILD_VIEW_FORM };
	enum Column { Filename, Size, Type, LastModified, Attributes };

	CReportListControl m_fileListCtrl;		// virtual list control

	// generated stuff
protected:
	virtual void DoDataExchange( CDataExchange* pDX );
protected:
	afx_msg void OnRClickFileList( NMHDR* pNMHDR, LRESULT* pResult );
	afx_msg void OnDblclkFileList( NMHDR* pNMHDR, LRESULT* pResult );
	afx_msg void OnGetDispInfoFileList( NMHDR* pNMHDR, LRESULT* pResult );
	afx_msg void OnViewMode( UINT cmdId );
	afx_msg void OnUpdateViewMode( CCmdUI* pCmdUI );
	afx_msg void OnUseCustomMenu( void );
	afx_msg void OnUpdateUseCustomMenu( CCmdUI* pCmdUI );

	DECLARE_MESSAGE_MAP()
};
