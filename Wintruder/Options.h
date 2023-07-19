#ifndef Options_h
#define Options_h
#pragma once

#include "utl/UI/RegistryOptions.h"


class CEnumTags;
class CAppService;


namespace opt
{
	enum FrameStyle { EntireWindow, NonClient, Frame };
	const CEnumTags& GetTags_FrameStyle( void );

	enum UpdateTarget { CurrentWnd, AtMouseWnd, ForegroundWnd, ActiveWnd, FocusedWnd, CapturedWnd, TopmostWnd, TopmostPopupWnd, TopmostVisibleWnd };
	const CEnumTags& GetTags_AutoUpdateTarget( void );

	enum QueryWndIcons { NoWndIcons, TopWndIcons, AllWndIcons };
	const CEnumTags& GetTags_QueryWndIcons( void );
}


struct COptions : public CRegistryOptions
{
	COptions( CAppService* pAppSvc );

	virtual void LoadAll( void );

	void PublishChangeEvent( void );
private:
	CAppService* m_pAppSvc;
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
protected:
	// base overrides
	virtual void OnOptionChanged( const void* pDataMember );
	virtual void OnUpdateOption( CCmdUI* pCmdUI );

	// generated command handlers
protected:
	DECLARE_MESSAGE_MAP()
};


#endif // Options_h
