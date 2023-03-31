#ifndef PathItemListCtrl_h
#define PathItemListCtrl_h
#pragma once

#include "ReportListControl.h"
#include "Path.h"


namespace ui
{
	inline ui::ISubjectAdapter* GetFullPathAdapter( void ) { return ui::CCodeAdapter::Instance(); }
}


// List control made of path items that implement utl::ISubject, typically CPathItemBase
class CPathItemListCtrl : public CReportListControl
{
public:
	CPathItemListCtrl( UINT columnLayoutId = 0, DWORD listStyleEx = lv::DefaultStyleEx );
	virtual ~CPathItemListCtrl();

	static CMenu& GetStdPathListPopupMenu( ListPopup popupType );

	COLORREF GetMissingFileColor( void ) const { return m_missingFileColor; }
	void SetMissingFileColor( COLORREF missingFileColor ) { m_missingFileColor = missingFileColor; }

	// selection
	template< typename PathType >
	bool QuerySelectedItemPaths( std::vector<PathType>& rSelFilePaths ) const;

	// base overrides
	virtual CMenu* GetPopupMenu( ListPopup popupType );
protected:
	virtual bool TrackContextMenu( ListPopup popupType, const CPoint& screenPos );
	virtual void CombineTextEffectAt( ui::CTextEffect& rTextEffect, LPARAM rowKey, int subItem, CListLikeCtrlBase* pCtrl ) const;	// ui::ITextEffectCallback interface
private:
	COLORREF m_missingFileColor;
	// generated stuff
public:
	virtual BOOL OnCmdMsg( UINT id, int code, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo );
protected:
	afx_msg BOOL OnLvnDblclk_Reflect( NMHDR* pNmHdr, LRESULT* pResult );
	afx_msg void OnCopyFilenames( void );
	afx_msg void OnCopyFolders( void );
	afx_msg void OnFileProperties( void );

	DECLARE_MESSAGE_MAP()
};


// template code

template< typename PathType >
bool CPathItemListCtrl::QuerySelectedItemPaths( std::vector<PathType>& rSelFilePaths ) const
{
	std::vector<utl::ISubject*> selItems;		// stands for CPathItemBase
	QuerySelectionAs( selItems );

	if ( selItems.empty() )
		return false;

	utl::QueryObjectCodes( rSelFilePaths, selItems );
	return true;
}


#endif // PathItemListCtrl_h
