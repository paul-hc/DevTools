#ifndef SearchPatternDialog_h
#define SearchPatternDialog_h
#pragma once

#include "AlbumModel.h"
#include "SearchPattern.h"
#include "utl/UI/HistoryComboBox.h"
#include "utl/UI/ui_fwd.h"


class CSearchPatternDialog : public CDialog
{
public:
	CSearchPatternDialog( const CSearchPattern& searchPattern, CWnd* pParent = NULL );
	virtual ~CSearchPatternDialog();
private:
	bool ValidateOK( ui::ComboField byField = ui::BySel );
public:
	CSearchPattern m_searchPattern;
private:
	// enum { IDD = IDD_SEARCH_PATTERN_DIALOG };
	CHistoryComboBox m_searchPathCombo;
	CHistoryComboBox m_wildFiltersCombo;

	// generated stuff
	protected:
	virtual void DoDataExchange( CDataExchange* pDX );	// DDX/DDV support
protected:
	virtual BOOL OnInitDialog();
	afx_msg void OnDropFiles( HDROP hDropInfo );
	afx_msg void CmBrowseFolder( void );
	afx_msg void OnCBnEditChangeSearchFolder();
	afx_msg void OnCBnSelChangeSearchFolder();
	afx_msg void CmBrowseFileButton();

	DECLARE_MESSAGE_MAP()
};


#endif // SearchPatternDialog_h
