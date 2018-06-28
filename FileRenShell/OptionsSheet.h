#ifndef OptionsSheet_h
#define OptionsSheet_h
#pragma once

#include "utl/DialogToolBar.h"
#include "utl/LayoutPropertySheet.h"
#include "utl/LayoutPropertyPage.h"


class COptionsSheet : public CLayoutPropertySheet
{
public:
	COptionsSheet( CWnd* pParent, UINT initialPageIndex = UINT_MAX );

	enum PageIndex { GeneralPage, CapitalizePage };
};


// property pages owned by COptionsSheet


class CEnumTags;
struct CGeneralOptions;


class CGeneralOptionsPage : public CLayoutPropertyPage
{
public:
	CGeneralOptionsPage( void );
	virtual ~CGeneralOptionsPage();
private:
	static const CEnumTags& GetTags_IconStdSize( void );
private:
	std::auto_ptr< CGeneralOptions > m_pOptions;

	// enum { IDD = IDD_OPTIONS_GENERAL_PAGE };
	CComboBox m_smallIconSizeCombo;
	CComboBox m_largeIconSizeCombo;

	// generated stuff
	protected:
	virtual void DoDataExchange( CDataExchange* pDX );
protected:
	afx_msg void OnBnClicked_ResetDefaultAll( void );

	DECLARE_MESSAGE_MAP()
};


#include "utl/ItemContentEdit.h"
#include "TitleCapitalizer.h"


class CCapitalizeOptionsPage : public CLayoutPropertyPage
{
public:
	CCapitalizeOptionsPage( void );
	virtual ~CCapitalizeOptionsPage();
private:
	CCapitalizeOptions m_options;
private:
	// enum { IDD = IDD_CAPITALIZE_OPTIONS_PAGE };
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

	// generated stuff
	protected:
	virtual void DoDataExchange( CDataExchange* pDX );
protected:
	afx_msg void OnBnClicked_ResetDefaultAll( void );

	DECLARE_MESSAGE_MAP()
};


#endif // OptionsSheet_h
