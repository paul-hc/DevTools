#pragma once

#include "utl/HistoryComboBox.h"
#include "utl/BaseMainDialog.h"
#include "ThemeStore.h"
#include "ThemeSampleStatic.h"


struct CThemeContext;


class CMainDialog : public CBaseMainDialog, private ISampleOptionsCallback
{
public:
	CMainDialog( void );
	~CMainDialog();
private:
	// ISampleOptionsCallback interface
	virtual CWnd* GetWnd( void ) ;
	virtual void RedrawSamples( void );

	CThemeContext GetSelThemeContext( void ) const;
	void SetupClassesCombo( void );
	void SetupPartsAndStatesTree( void );
	void OutputCurrentTheme( void );
private:
	CThemeSampleOptions m_options;
	CThemeStore m_themeStore;
	int m_internalChange;
private:
	// enum { IDD = IDD_MAIN_DIALOG };

	CComboBox m_classCombo;
	CTreeCtrl m_partStateTree;
	CComboBox m_classFilterCombo;
	CComboBox m_partsFilterCombo;
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
	afx_msg void OnCbnSelChange_ClassCombo( void );
	afx_msg void OnTVnSelChanged_PartStateTree( NMHDR* pNmHdr, LRESULT* pResult );
	afx_msg void OnTvnCustomDraw_PartStateTree( NMHDR* pNmHdr, LRESULT* pResult );
	afx_msg void OnCbnSelChange_ClassFilterCombo( void );
	afx_msg void OnCbnSelChange_PartsFilterCombo( void );

	DECLARE_MESSAGE_MAP()
};