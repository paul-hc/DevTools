#ifndef PathItemListCtrl_h
#define PathItemListCtrl_h
#pragma once

#include "ReportListControl.h"
#include "Path.h"


class CPathItemBase;
class CShellContextMenuHost;


class CPathItemListCtrl : public CReportListControl
{
public:
	CPathItemListCtrl( UINT columnLayoutId = 0, DWORD listStyleEx = lv::DefaultStyleEx );
	virtual ~CPathItemListCtrl();

	enum ShellContextMenuStyle { NoShellMenu, ExplorerSubMenu, ShellMenuFirst, ShellMenuLast };

	bool UseShellContextMenu( void ) const { return m_cmStyle != NoShellMenu; }
	void SetShellContextMenuStyle( ShellContextMenuStyle cmStyle, UINT cmQueryFlags = UINT_MAX );

	bool IsShellMenuCmd( int cmdId ) const;

	static CMenu& GetStdPathListPopupMenu( ListPopup popupType );

	// selection
	template< typename PathType >
	bool QuerySelectedItemPaths( std::vector< PathType >& rSelFilePaths ) const;

	// base overrides
	virtual bool IsInternalCmdId( int cmdId ) const;
	virtual CMenu* GetPopupMenu( ListPopup popupType );
protected:
	virtual bool TrackContextMenu( ListPopup popupType, const CPoint& screenPos );
private:
	CMenu* MakeContextMenuHost( CMenu* pSrcPopupMenu, const std::vector< std::tstring >& selFilePaths );
	void ResetShellContextMenu( void );
private:
	ShellContextMenuStyle m_cmStyle;
	UINT m_cmQueryFlags;
	std::auto_ptr< CShellContextMenuHost > m_pShellMenuHost;

	// generated stuff
public:
	virtual BOOL OnCmdMsg( UINT id, int code, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo );
protected:
	afx_msg BOOL OnLvnDblclk_Reflect( NMHDR* pNmHdr, LRESULT* pResult );
	afx_msg void OnCopyFilename( void );
	afx_msg void OnCopyParentDirPath( void );
	afx_msg void OnFileProperties( void );
	afx_msg void OnEditListViewProperties( void );

	DECLARE_MESSAGE_MAP()
};


// template code

template< typename PathType >
bool CPathItemListCtrl::QuerySelectedItemPaths( std::vector< PathType >& rSelFilePaths ) const
{
	std::vector< CPathItemBase* > selItems;
	QuerySelectionAs( selItems );

	if ( selItems.empty() )
		return false;

	utl::QueryObjectCodes( rSelFilePaths, selItems );
	return true;
}


#endif // PathItemListCtrl_h
