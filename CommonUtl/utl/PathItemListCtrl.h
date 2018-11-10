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
	CPathItemListCtrl( UINT columnLayoutId = 0, DWORD listStyleEx = DefaultStyleEx );
	virtual ~CPathItemListCtrl();

	enum ShellContextMenuStyle { NoShellMenu, ExplorerSubMenu, ShellMenuFirst, ShellMenuLast };

	bool UseShellContextMenu( void ) const { return m_cmStyle != NoShellMenu; }
	void SetShellContextMenuStyle( ShellContextMenuStyle cmStyle, UINT cmQueryFlags = UINT_MAX );

	bool IsShellMenuCmd( int cmdId ) const;

	// selection
	template< typename PathType >
	void QuerySelectedItemPaths( std::vector< PathType >& rSelFilePaths ) const;

	// base overrides
	virtual bool IsInternalCmdId( int cmdId ) const;
	virtual CMenu* GetPopupMenu( ListPopup popupType );
protected:
	virtual bool TrackContextMenu( ListPopup popupType, const CPoint& screenPos );
private:
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

	DECLARE_MESSAGE_MAP()
};


// template code

template< typename PathType >
void CPathItemListCtrl::QuerySelectedItemPaths( std::vector< PathType >& rSelFilePaths ) const
{
	std::vector< CPathItemBase* > selItems;
	QuerySelectionAs( selItems );

	if ( !selItems.empty() )
		utl::QueryObjectCodes( rSelFilePaths, selItems );
}


#endif // PathItemListCtrl_h
