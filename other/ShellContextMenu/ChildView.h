#pragma once


enum ListViewMode { LargeIconView, SmallIconView, ListView, ReportView };

struct CFileItemInfo;


class CChildView : public CWnd
{
public:
	CChildView( void );
	virtual ~CChildView();
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
	CListCtrl m_fileListCtrl;			// virtual list control

	// generated stuff
protected:
	virtual BOOL PreCreateWindow( CREATESTRUCT& cs );
protected:
	afx_msg int OnCreate( LPCREATESTRUCT lpCreateStruct );
	afx_msg void OnSize( UINT nType, int cx, int cy );
	afx_msg void OnPaint();
	afx_msg void OnRClickFileList( NMHDR* pNMHDR, LRESULT* pResult );
	afx_msg void OnDblclkFileList( NMHDR* pNMHDR, LRESULT* pResult );
	afx_msg void OnGetDispInfoFileList( NMHDR* pNMHDR, LRESULT* pResult );

	DECLARE_MESSAGE_MAP()
private:
};
