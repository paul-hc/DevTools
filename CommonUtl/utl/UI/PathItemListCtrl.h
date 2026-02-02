#ifndef PathItemListCtrl_h
#define PathItemListCtrl_h
#pragma once

#include "utl/Path.h"
#include "ReportListControl.h"
#include "SubjectAdapter.h"


// List control made of path items that implement utl::ISubject, typically CPathItemBase
class CPathItemListCtrl : public CReportListControl
{
public:
	CPathItemListCtrl( UINT columnLayoutId = 0, DWORD listStyleEx = lv::DefaultStyleEx );
	virtual ~CPathItemListCtrl();

	static CMenu& GetStdPathListPopupMenu( ListPopup popupType );
	static fs::CPath AsPath( const utl::ISubject* pObject );		// expands environment variables ("%WIN_VAR%" and "$(VC_MACRO_VAR)")

	COLORREF GetMissingFileColor( void ) const { return m_missingFileColor; }
	void SetMissingFileColor( COLORREF missingFileColor ) { m_missingFileColor = missingFileColor; }

	// selection
	template< typename PathType >
	bool QuerySelectedItemPaths( std::vector<PathType>& rSelFilePaths, bool expanded = true ) const;

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
bool CPathItemListCtrl::QuerySelectedItemPaths( std::vector<PathType>& rSelFilePaths, bool expanded /*= true*/ ) const
{
	std::vector<utl::ISubject*> selItems;		// stands for CPathItemBase
	QuerySelectionAs( selItems );

	if ( selItems.empty() )
		return false;

	utl::QueryObjectCodes( rSelFilePaths, selItems );

	if ( expanded )
		std::for_each( rSelFilePaths.begin(), rSelFilePaths.end(), func::ExpandPath() );

	return true;
}


#endif // PathItemListCtrl_h
