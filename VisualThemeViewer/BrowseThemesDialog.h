#ifndef BrowseThemesDialog_h
#define BrowseThemesDialog_h
#pragma once

#include "utl/BaseMainDialog.h"
#include "utl/TreeControl.h"
#include "ThemeStore_fwd.h"


namespace hlp
{
	std::tstring FormatCounts( size_t count, size_t total );
}


class COptions;
class CThemeCustomDraw;


class CBrowseThemesDialog
	: public CBaseMainDialog			// display an icon, persist window placement in maximized state
	, private ui::ITextEffectCallback
{
public:
	CBrowseThemesDialog( const COptions* pOptions, const CThemeStore* pThemeStore, Relevance relevanceFilter, CWnd* pParent );
	virtual ~CBrowseThemesDialog();

	void SetSelectedNode( IThemeNode* pSelNode ) { m_pSelNode = pSelNode; }
	IThemeNode* GetSelectedNode( void ) const { return m_pSelNode; }
	CThemeClass* GetSelectedClass( void ) const;
private:
	// ui::ITextEffectCallback interface
	virtual void CombineTextEffectAt( ui::CTextEffect& rTextEffect, LPARAM rowKey, int subItem, CListLikeCtrlBase* pCtrl ) const;

	void SetupTree( void );
private:
	const COptions* m_pOptions;
	const CThemeStore* m_pThemeStore;
	Relevance m_relevanceFilter;
	std::auto_ptr< CThemeCustomDraw > m_pTreeCustomDraw;

	IThemeNode* m_pSelNode;
private:
	// enum { IDD = IDD_BROWSE_THEMES_DIALOG };

	CComboBox m_filterCombo;
	CTreeControl m_themesTree;

	// generated overrides
	protected:
	virtual void DoDataExchange( CDataExchange* pDX );
protected:
	afx_msg void OnCbnSelChange_FilterCombo( void );
	afx_msg void OnTvnSelChanged_ThemesTree( NMHDR* pNmHdr, LRESULT* pResult );

	DECLARE_MESSAGE_MAP()
};


#endif // BrowseThemesDialog_h
