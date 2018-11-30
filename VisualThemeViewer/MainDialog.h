#pragma once

#include "utl/BaseMainDialog.h"
#include "utl/DialogToolBar.h"
#include "utl/HistoryComboBox.h"
#include "utl/ReportListControl.h"
#include "utl/TreeControl.h"
#include "ThemeStore.h"
#include "ThemeSampleStatic.h"


struct CThemeContext;
class CThemeCustomDraw;


class CMainDialog
	: public CBaseMainDialog
	, private ISampleOptionsCallback
	, private ui::ITextEffectCallback
{
public:
	CMainDialog( void );
	virtual ~CMainDialog();
private:
	// ISampleOptionsCallback interface
	virtual CWnd* GetWnd( void ) ;
	virtual void RedrawSamples( void );

	// ui::ITextEffectCallback interface
	virtual void CombineTextEffectAt( ui::CTextEffect& rTextEffect, LPARAM rowKey, int subItem, CListLikeCtrlBase* pCtrl ) const;

	CThemeContext GetSelThemeContext( void ) const;
	void SetupClassesList( void );
	void SetupPartsAndStatesTree( void );
	void OutputCurrentTheme( void );

	static std::tstring FormatCounts( unsigned int count, unsigned int total );
private:
	CThemeSampleOptions m_options;
	CThemeStore m_themeStore;
	std::auto_ptr< CThemeCustomDraw > m_pCustomDraw;
private:
	// enum { IDD = IDD_MAIN_DIALOG };
	enum ClassColumn { ClassName, RelevanceTag };

	CReportListControl m_classList;
	CTreeControl m_partStateTree;
	CComboBox m_classFilterCombo;
	CComboBox m_partsFilterCombo;
	CDialogToolBar m_toolbar;
	CHistoryComboBox m_bkColorCombo;

	enum { Tiny, Small, Medium, Large, SampleCount };
	CThemeSampleStatic m_samples[ SampleCount ];
protected:
	// generated overrides
	public:
	virtual BOOL OnCmdMsg( UINT id, int code, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo );
	protected:
	virtual void DoDataExchange( CDataExchange* pDX );
protected:
	// message map functions
	afx_msg void OnDestroy( void );
	afx_msg void OnLvnItemChanged_ThemeClass( NMHDR* pNmHdr, LRESULT* pResult );
	afx_msg void OnTvnSelChanged_PartStateTree( NMHDR* pNmHdr, LRESULT* pResult );
	afx_msg void OnCbnSelChange_ClassFilterCombo( void );
	afx_msg void OnCbnSelChange_PartsFilterCombo( void );
	afx_msg void OnEditCopy( void );
	afx_msg void OnUpdateEditCopy( CCmdUI* pCmdUI );
	afx_msg void OnCopyTheme( UINT cmdId );
	afx_msg void OnUpdateCopyTheme( CCmdUI* pCmdUI );

	DECLARE_MESSAGE_MAP()
};
