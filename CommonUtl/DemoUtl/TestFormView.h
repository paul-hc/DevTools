#pragma once

#include "utl/UI/LayoutFormView.h"
#include "utl/UI/EnumSplitButton.h"
#include "ITestMarkup.h"


class CDemoTemplate;


class CTestFormView : public CLayoutFormView
	, public ITestMarkup
{
	DECLARE_DYNCREATE( CTestFormView )
protected:
	CTestFormView( void );
public:
	virtual ~CTestFormView();

	// ui::ICustomCmdInfo interface
	virtual void QueryTooltipText( std::tstring& rText, UINT cmdId, CToolTipCtrl* pTooltip ) const;

	CTestDoc* GetDocument( void ) const { return reinterpret_cast<CTestDoc*>( m_pDocument ); }
private:
	// enum { IDD = IDD_DEMO_FORM };

	std::auto_ptr<CDemoTemplate> m_pDemo;
	CEnumSplitButton m_miscDlgButton;
private:
	enum MiscDialog { ToolbarDialog, TestColorsDialog };

	const CEnumTags& GetTags_MiscDialog( void );
protected:
	// base overrides
	virtual void OnIdleUpdateControls( void );

	// generated stuff
public:
	virtual BOOL PreCreateWindow( CREATESTRUCT& cs );
	virtual BOOL OnCmdMsg( UINT id, int code, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo );
protected:
	virtual void DoDataExchange( CDataExchange* pDX );
	virtual void OnInitialUpdate( void );
protected:
	afx_msg void OnRunImageUnitTests( void );
	afx_msg void OnStudyImage( void );
	afx_msg void OnStudyListDiffs( void );
	afx_msg void OnStudyFileChecksums( void );
	afx_msg void OnStudyBuddyControls( void );
	afx_msg void OnStudyTaskDialog( void );
	afx_msg void OnStudyMiscDialog( void );
	afx_msg void OnToggle_ModelessBuddyDlg( void );

	DECLARE_MESSAGE_MAP()
};
