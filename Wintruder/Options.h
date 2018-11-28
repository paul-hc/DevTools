#ifndef Options_h
#define Options_h
#pragma once

#include "utl/OptionContainer.h"


class CEnumTags;
class CAppService;


namespace opt
{
	enum FrameStyle { EntireWindow, NonClient, Frame };
	const CEnumTags& GetTags_FrameStyle( void );

	enum UpdateTarget { CurrentWnd, AtMouseWnd, ForegroundWnd, ActiveWnd, FocusWnd, TopmostWnd, TopmostPopupWnd, TopmostVisibleWnd };
	const CEnumTags& GetTags_AutoUpdateTarget( void );

	enum QueryWndIcons { NoWndIcons, TopWndIcons, AllWndIcons };
	const CEnumTags& GetTags_QueryWndIcons( void );
}


struct COptions : public CCmdTarget
{
	COptions( CAppService* pAppSvc );

	void Load( void );
	void Save( void ) const;

	void PublishChangeEvent( void );

	template< typename ValueT >
	bool ModifyOption( ValueT* pOptionDataMember, const ValueT& newValue );

	void ToggleOption( bool* pBoolDataMember ) { ModifyOption( pBoolDataMember, !*pBoolDataMember ); }
private:
	CAppService* m_pAppSvc;
	reg::COptionContainer m_regOptions;
public:
	const bool m_hasUIPI;					// Windows 8+?

	bool m_keepTopmost;
	bool m_hideOnTrack;
	bool m_autoHighlight;
	bool m_ignoreHidden;
	bool m_ignoreDisabled;
	bool m_displayZeroFlags;

	opt::FrameStyle m_frameStyle;
	int m_frameSize;
	opt::QueryWndIcons m_queryWndIcons;		// allows control over icon access deadlocks

	bool m_autoUpdate;
	bool m_autoUpdateRefresh;
	int m_autoUpdateTimeout;
	opt::UpdateTarget m_updateTarget;

	// generated command handlers
protected:
	afx_msg void OnToggle_KeepTopmost( void );
	afx_msg void OnUpdate_KeepTopmost( CCmdUI* pCmdUI );
	afx_msg void OnToggle_AutoHideCheck( void );
	afx_msg void OnUpdate_AutoHideCheck( CCmdUI* pCmdUI );
	afx_msg void OnToggle_AutoHilightCheck( void );
	afx_msg void OnUpdate_AutoHilightCheck( CCmdUI* pCmdUI );
	afx_msg void OnToggle_IgnoreHidden( void );
	afx_msg void OnUpdate_IgnoreHidden( CCmdUI* pCmdUI );
	afx_msg void OnToggle_IgnoreDisabled( void );
	afx_msg void OnUpdate_IgnoreDisabled( CCmdUI* pCmdUI );
	afx_msg void OnToggle_DisplayZeroFlags( void );
	afx_msg void OnUpdate_DisplayZeroFlags( CCmdUI* pCmdUI );
	afx_msg void OnToggle_AutoUpdate( void );
	afx_msg void OnUpdate_AutoUpdate( CCmdUI* pCmdUI );
	afx_msg void OnToggle_AutoUpdateRefresh( void );
	afx_msg void OnUpdate_AutoUpdateRefresh( CCmdUI* pCmdUI );

	DECLARE_MESSAGE_MAP()
};


// COptions template code

template< typename ValueT >
inline bool COptions::ModifyOption( ValueT* pOptionDataMember, const ValueT& newValue )
{
	if ( !m_regOptions.ModifyOption( pOptionDataMember, newValue, true ) )		// save it right away if changed
		return false;

	PublishChangeEvent();
	return true;
}


#endif // Options_h
