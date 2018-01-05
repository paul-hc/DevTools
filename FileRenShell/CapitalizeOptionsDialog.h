#ifndef CapitalizeOptionsDialog_h
#define CapitalizeOptionsDialog_h
#pragma once

#include "utl/LayoutDialog.h"
#include "utl/ItemContentEdit.h"
#include "TitleCapitalizer.h"


class CCapitalizeOptionsDialog : public CLayoutDialog
{
public:
	CCapitalizeOptionsDialog( CWnd* pParent = NULL );
	virtual ~CCapitalizeOptionsDialog();
private:
	CCapitalizeOptions m_options;

	//enum { IDD = IDD_CAPITALIZE_OPTIONS };
	CTextEdit m_wordBreakCharsEdit;
	CItemListEdit m_wordBreakPrefixesEdit;
	CItemListEdit m_alwaysPreserveWordsEdit;
	CItemListEdit m_alwaysUppercaseWordsEdit;
	CItemListEdit m_alwaysLowercaseWordsEdit;
	CItemListEdit m_articlesEdit;
	CComboBox m_articlesCombo;
	CItemListEdit m_conjunctionsEdit;
	CComboBox m_conjunctionsCombo;
	CItemListEdit m_prepositionsEdit;
	CComboBox m_prepositionsCombo;
protected:
	// generated overrides
	protected:
	virtual void DoDataExchange( CDataExchange* pDX );
protected:
	// generated message map
	afx_msg void OnBnClicked_ResetDefaultAll( void );

	DECLARE_MESSAGE_MAP()
};


#endif // CapitalizeOptionsDialog_h
