#ifndef DemoTemplate_h
#define DemoTemplate_h
#pragma once

#include <vector>
#include "utl/LayoutChildPropertySheet.h"
#include "utl/LayoutPropertyPage.h"
#include "utl/ReportListControl.h"
#include "utl/SplitPushButton.h"
#include "utl/SpinEdit.h"
#include "utl/EnumSplitButton.h"
#include "utl/ThemeStatic.h"


enum ResizeStyle;


class CDemoTemplate : public CCmdTarget
{
public:
	CDemoTemplate( CWnd* pOwner );
	virtual ~CDemoTemplate();

	// ui::ICmdCallback interface
	virtual void QueryTooltipText( std::tstring& rText, UINT cmdId, CToolTipCtrl* pTooltip ) const;

	virtual void DoDataExchange( CDataExchange* pDX );
	virtual BOOL OnCmdMsg( UINT id, int code, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo );

	static const std::vector< std::tstring >& GetTextItems( void );
private:
	enum { MaxDemoDepth = 3 };
	static ResizeStyle m_dialogResizeStyle;

	CWnd* m_pOwner;
	ui::ILayoutEngine* m_pLayoutEngine;
	UINT m_selRadio;
public:
	CEnumSplitButton m_dialogButton;
	CComboBox m_formatCombo;
	CPickMenuStatic m_pickFormatStatic;
	CPickMenuStatic m_pickFormatCheckedStatic;		// with checked menu images
	CSpinEdit m_counterEdit;
	CIconButton m_resetSeqCounterButton;
	CLayoutChildPropertySheet m_detailSheet;
	CSplitPushButton m_capitalizeButton;
	CEnumSplitButton m_changeCaseButton;
	CComboBox m_delimiterSetCombo;
	CEdit m_newDelimiterEdit;
	CThemeStatic m_delimStatic;
private:
	// message map functions
	afx_msg void OnBnClicked_OpenDialog( void );
	afx_msg void OnBnClicked_OpenPropertySheet( void );
	afx_msg void OnBnClicked_CreatePropertySheet( void );
	afx_msg void OnToggle_DisableSmoothResize( void );
	afx_msg void OnToggle_DisableThemes( void );
	afx_msg void OnBnClicked_DropFormat( void );
	afx_msg void OnBnClicked_DropDownFormat( void );
	afx_msg void OnBnClicked_CapitalizeDestFiles( void );
	afx_msg void OnBnClicked_CapitalizeOptions( void );
	afx_msg void OnNumSequence( UINT cmdId );
	afx_msg void OnUpdateNumSequence( CCmdUI* pCmdUI );
	afx_msg void OnDropAlignCheckedPicker( UINT cmdId );
	afx_msg void OnUpdateDropAlignCheckedPicker( CCmdUI* pCmdUI );

	DECLARE_MESSAGE_MAP()
};


class CListPage : public CLayoutPropertyPage
{
public:
	CListPage( void );
	virtual ~CListPage() {}
private:
	enum Column { Source, Destination, Notes };
	CReportListControl m_fileListView;
protected:
	virtual void DoDataExchange( CDataExchange* pDX );
	afx_msg void OnToggle_UseAlternateRows( void );
	afx_msg void OnToggle_UseTextEffects( void );

	DECLARE_MESSAGE_MAP()
};


class CEditPage : public CLayoutPropertyPage
{
public:
	CEditPage( void );
	virtual ~CEditPage() {}

	virtual void QueryTooltipText( std::tstring& rText, UINT cmdId, CToolTipCtrl* pTooltip ) const;
protected:
	virtual void DoDataExchange( CDataExchange* pDX );
private:
	CEdit m_sourceEdit;
	CEdit m_destEdit;
	CComboBox m_sourceCombo;
	CComboBox m_destCombo;
};


class CDemoPage : public CLayoutPropertyPage
{
public:
	CDemoPage( void );
	virtual ~CDemoPage();

	// ui::ICmdCallback interface
	virtual void QueryTooltipText( std::tstring& rText, UINT cmdId, CToolTipCtrl* pTooltip ) const { m_pDemo->QueryTooltipText( rText, cmdId, pTooltip ); }
	virtual BOOL OnCmdMsg( UINT id, int code, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo );
protected:
	virtual void DoDataExchange( CDataExchange* pDX );
private:
	enum { MaxDepth = 3 };
	std::auto_ptr< CDemoTemplate > m_pDemo;
};


class CDetailsPage : public CLayoutPropertyPage
{
public:
	CDetailsPage( void );
	virtual ~CDetailsPage();
protected:
	virtual void DoDataExchange( CDataExchange* pDX );
private:
	enum { MaxDepth = 3 };
	CLayoutChildPropertySheet m_detailSheet;
};


#endif // DemoTemplate_h
