#ifndef SearchPatternDialog_h
#define SearchPatternDialog_h
#pragma once

#include "AlbumModel.h"
#include "SearchPattern.h"
#include "utl/UI/LayoutDialog.h"
#include "utl/UI/HistoryComboBox.h"
#include "utl/UI/ItemContentHistoryCombo.h"
#include "utl/UI/EnumComboBox.h"
#include "utl/UI/ui_fwd.h"


class CSearchPatternDialog : public CLayoutDialog
{
public:
	CSearchPatternDialog( const CSearchPattern* pSrcPattern, CWnd* pParent = NULL );
	virtual ~CSearchPatternDialog();
private:
	void ValidatePattern( ui::ComboField byField = ui::BySel );
public:
	std::auto_ptr<CSearchPattern> m_pSearchPattern;
private:
	// enum { IDD = IDD_SEARCH_PATTERN_DIALOG };

	CItemContentHistoryCombo m_searchPathCombo;
	CHistoryComboBox m_wildFiltersCombo;
	CEnumComboBox m_searchModeCombo;

	// generated stuff
protected:
	virtual void DoDataExchange( CDataExchange* pDX );		// DDX/DDV support
protected:
	afx_msg void OnDropFiles( HDROP hDropInfo );
	afx_msg void OnBrowseFolder( void );
	afx_msg void OnBrowseFile( void );
	afx_msg void OnUpdate_BrowsePath( CCmdUI* pCmdUI );
	afx_msg void OnCBnEditChange_SearchFolder( void );
	afx_msg void OnCBnSelChange_SearchFolder( void );
	afx_msg void OnCBnEditChange_WildFilters( void );
	afx_msg void OnCBnSelChange_WildFilters( void );
	afx_msg void OnCBnSelChange_SearchMode( void );

	DECLARE_MESSAGE_MAP()
};


#endif // SearchPatternDialog_h
