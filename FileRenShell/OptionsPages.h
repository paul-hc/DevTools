#ifndef OptionsPages_h
#define OptionsPages_h
#pragma once

#include "utl/UI/LayoutPropertyPage.h"
#include "utl/UI/DialogToolBar.h"


// property pages owned by COptionsSheet


class CEnumTags;


#include "GeneralOptions.h"


class CGeneralOptionsPage : public CLayoutPropertyPage
{
public:
	CGeneralOptionsPage( void );
	virtual ~CGeneralOptionsPage();

	// base overrides
	virtual void ApplyPageChanges( void ) throws_( CRuntimeException );
private:
	void UpdateStatus( void );

	static const CEnumTags& GetTags_IconDim( void );
private:
	CGeneralOptions m_options;
	static const CGeneralOptions s_defaultOptions;
private:
	// enum { IDD = IDD_OPTIONS_GENERAL_PAGE };
	CComboBox m_smallIconDimCombo;
	CComboBox m_largeIconDimCombo;

	// generated stuff
	protected:
	virtual void DoDataExchange( CDataExchange* pDX );
protected:
	virtual void OnFieldModified( void );			// base override
	afx_msg void OnBnClicked_ResetDefaultAll( void );

	DECLARE_MESSAGE_MAP()
};


#include "utl/UI/ItemContentEdit.h"
#include "TitleCapitalizer.h"


class CCapitalizeOptionsPage : public CLayoutPropertyPage
{
public:
	CCapitalizeOptionsPage( void );
	virtual ~CCapitalizeOptionsPage();

	// base overrides
	virtual void ApplyPageChanges( void ) throws_( CRuntimeException );
private:
	void UpdateStatus( void );
private:
	CCapitalizeOptions m_options;
	static const CCapitalizeOptions s_defaultOptions;

	// enum { IDD = IDD_OPTIONS_CAPITALIZE_PAGE };
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
	virtual void OnFieldModified( void );			// base override
	afx_msg void OnBnClicked_ResetDefaultAll( void );

	DECLARE_MESSAGE_MAP()
};


#endif // OptionsPages_h
