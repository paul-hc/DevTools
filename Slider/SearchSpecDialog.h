#ifndef SearchSpecDialog_h
#define SearchSpecDialog_h
#pragma once

#include "FileList.h"
#include "utl/HistoryComboBox.h"
#include "utl/ui_fwd.h"


class CSearchSpecDialog : public CDialog
{
public:
	CSearchSpecDialog( const CSearchSpec& searchSpec, CWnd* pParent = NULL );
	virtual ~CSearchSpecDialog();
private:
	bool ValidateOK( ui::ComboField byField = ui::BySel );
public:
	CSearchSpec m_searchSpec;
private:
	// enum { IDD = IDD_SEARCH_SPEC_DIALOG };
	CHistoryComboBox m_searchPathCombo;
	CHistoryComboBox m_searchFiltersCombo;

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


#endif // SearchSpecDialog_h
